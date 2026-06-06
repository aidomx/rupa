#include <rupa.h>

struct termios rupaterm;

int enableRawMode(void) {
  if (tcgetattr(STDIN_FILENO, &rupaterm) == -1)
    return -1;

  atexit(disableRawMode);

  struct termios raw = rupaterm;

  raw.c_lflag &= ~(ECHO | ICANON | ISIG | ICRNL);
  raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  /*raw.c_oflag &= OPOST;*/
  return tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disableRawMode(void) {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &rupaterm) == -1) {
    perror("tcsetattr");
    exit(1);
  }
}

int getEditorKey(State *state) {
  if (!state)
    return -1;

  Editor *ed = state->repl->editor;
  int reader = read(STDIN_FILENO, ed->sequence, 1);
  if (!reader)
    return -1;

  return ed->sequence[0];
}

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1 && ws.ws_col != 0) {
    *rows = ws.ws_row;
    *cols = ws.ws_col;
    return 0;
  }

  if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
    return -1;
  }

  return getCusorPosition(rows, cols);
}

int getCusorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;

  if (write(STDOUT_FILENO, "\1b[6n", 4) != 4) {
    return -1;
  }

  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) {
      break;
    }

    if (buf[i] == 'R')
      break;
    i++;
  }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[') {
    return -1;
  }

  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) {
    return -1;
  }

  return 0;
}

int getWindowSizeFallback(int *rows, int *cols) {
  *rows = 24;
  *cols = 80;

  char *envRows = getenv("LINES");
  char *envCols = getenv("COLUMNS");

  if (envRows)
    *rows = atoi(envRows);
  if (envCols)
    *cols = atoi(envCols);

  return 0;
}
