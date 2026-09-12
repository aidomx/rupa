#include <rupa.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

/* ==================== Validation helpers ==================== */

static InterpreterResult netError(Error *error, const char *name, const char *message) {
  if (error)
    addError(error, (ErrorInfo){.code = (char *)"NetError",
                                .message = (char *)message,
                                .line = 0,
                                .row = 0,
                                .type = ERR_INTERNAL});
  (void)name;
  return resultFlow(FLOW_ERROR, valueNull());
}

/* ==================== net.connect(host, port) ==================== */
/* Create TCP connection to host:port, returns socket fd */

static InterpreterResult netConnect(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 2) return netError(error, "connect", "net.connect() expects host and port");

  if (argv[0].type != VALUE_STRING || !argv[0].as.string)
    return netError(error, "connect", "net.connect() host must be string");

  int port;
  if (argv[1].type == VALUE_NUMBER)
    port = argv[1].as.number;
  else
    return netError(error, "connect", "net.connect() port must be number");

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) return netError(error, "connect", "net.connect() socket creation failed");

  struct hostent *server = gethostbyname(argv[0].as.string);
  if (!server) {
    close(sockfd);
    return netError(error, "connect", "net.connect() host not found");
  }

  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  serv_addr.sin_port = htons(port);

  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    close(sockfd);
    return netError(error, "connect", "net.connect() connection failed");
  }

  return resultNormal(valueNumber(sockfd));
}

/* ==================== net.send(fd, data) ==================== */

static InterpreterResult netSend(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 2) return netError(error, "send", "net.send() expects fd and data");

  int fd;
  if (argv[0].type == VALUE_NUMBER)
    fd = argv[0].as.number;
  else
    return netError(error, "send", "net.send() fd must be number");

  if (argv[1].type != VALUE_STRING || !argv[1].as.string)
    return netError(error, "send", "net.send() data must be string");

  ssize_t sent = send(fd, argv[1].as.string, strlen(argv[1].as.string), 0);
  if (sent < 0) return netError(error, "send", "net.send() failed");

  return resultNormal(valueNumber((int)sent));
}

/* ==================== net.receive(fd, size?) ==================== */

static InterpreterResult netReceive(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1) return netError(error, "receive", "net.receive() expects fd");

  int fd;
  if (argv[0].type == VALUE_NUMBER)
    fd = argv[0].as.number;
  else
    return netError(error, "receive", "net.receive() fd must be number");

  int bufsize = 4096;
  if (argc >= 2 && argv[1].type == VALUE_NUMBER) bufsize = argv[1].as.number;

  char *buf = gcmall(bufsize + 1);
  ssize_t n = recv(fd, buf, bufsize, 0);
  if (n < 0) {
    gcfree(buf);
    return netError(error, "receive", "net.receive() failed");
  }
  buf[n] = '\0';

  return resultNormal(valueString(buf));
}

/* ==================== net.close(fd) ==================== */

static InterpreterResult netClose(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1) return netError(error, "close", "net.close() expects fd");

  int fd;
  if (argv[0].type == VALUE_NUMBER)
    fd = argv[0].as.number;
  else
    return netError(error, "close", "net.close() fd must be number");

  close(fd);
  return resultNormal(valueBoolean(true));
}

/* ==================== net.listen(port, backlog?) ==================== */
/* Create TCP server socket, returns fd */

static InterpreterResult netListen(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1) return netError(error, "listen", "net.listen() expects port");

  int port;
  if (argv[0].type == VALUE_NUMBER)
    port = argv[0].as.number;
  else
    return netError(error, "listen", "net.listen() port must be number");

  int backlog = 5;
  if (argc >= 2 && argv[1].type == VALUE_NUMBER) backlog = argv[1].as.number;

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) return netError(error, "listen", "net.listen() socket creation failed");

  int opt = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port);

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    close(sockfd);
    return netError(error, "listen", "net.listen() bind failed");
  }

  if (listen(sockfd, backlog) < 0) {
    close(sockfd);
    return netError(error, "listen", "net.listen() listen failed");
  }

  return resultNormal(valueNumber(sockfd));
}

/* ==================== net.accept(fd) ==================== */
/* Accept connection, returns {fd, address} */

static InterpreterResult netAccept(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1) return netError(error, "accept", "net.accept() expects fd");

  int fd;
  if (argv[0].type == VALUE_NUMBER)
    fd = argv[0].as.number;
  else
    return netError(error, "accept", "net.accept() fd must be number");

  struct sockaddr_in cli_addr;
  socklen_t clilen = sizeof(cli_addr);
  int newfd = accept(fd, (struct sockaddr *)&cli_addr, &clilen);
  if (newfd < 0) return netError(error, "accept", "net.accept() failed");

  RuntimeValue result = valueObject(NULL);
  valueObjectSet(&result, "fd", valueNumber(newfd));
  valueObjectSet(&result, "address", valueString(inet_ntoa(cli_addr.sin_addr)));
  valueObjectSet(&result, "port", valueNumber(ntohs(cli_addr.sin_port)));

  return resultNormal(result);
}

/* ==================== net.resolve(host) ==================== */
/* Resolve hostname to IP address */

static InterpreterResult netResolve(int argc, RuntimeValue *argv, RuntimeEnv *env, Error *error) {
  (void)env;
  if (argc < 1 || argv[0].type != VALUE_STRING || !argv[0].as.string)
    return netError(error, "resolve", "net.resolve() expects hostname");

  struct hostent *server = gethostbyname(argv[0].as.string);
  if (!server) return netError(error, "resolve", "net.resolve() host not found");

  char *ip = inet_ntoa(*(struct in_addr *)server->h_addr);
  return resultNormal(valueString(gcstrdup(ip)));
}

/* ==================== Module init ==================== */

InterpreterResult stdNetInit(Node *node, int id, RuntimeEnv *env, Error *error) {
  (void)node;
  (void)id;
  (void)env;
  (void)error;

  RuntimeValue mod = valueObject(NULL);
  RuntimeValue fn;

  fn = valueNativeFunction("connect", netConnect, 2);
  valueObjectSet(&mod, "connect", fn);

  fn = valueNativeFunction("send", netSend, 2);
  valueObjectSet(&mod, "send", fn);

  fn = valueNativeFunction("receive", netReceive, 1);
  valueObjectSet(&mod, "receive", fn);

  fn = valueNativeFunction("close", netClose, 1);
  valueObjectSet(&mod, "close", fn);

  fn = valueNativeFunction("listen", netListen, 1);
  valueObjectSet(&mod, "listen", fn);

  fn = valueNativeFunction("accept", netAccept, 1);
  valueObjectSet(&mod, "accept", fn);

  fn = valueNativeFunction("resolve", netResolve, 1);
  valueObjectSet(&mod, "resolve", fn);

  return resultNormal(mod);
}
