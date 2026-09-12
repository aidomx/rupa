# DateTime Module Grammar

## AST Structure

### `datetime.now()`

```text
Call:
  Callee:
    Member:
      Object:
        Identifier: datetime
      Member:
        Identifier: now
```

### `datetime.format(ts, fmt)`

```text
Call:
  Callee:
    Member:
      Object:
        Identifier: datetime
      Member:
        Identifier: format
  Arg 1:
    Identifier: ts
  Arg 2:
    String: "%Y-%m-%d"
```

## Module Structure

```text
datetime
├── now()          — current Unix timestamp
├── nowMs()        — current timestamp in ms
├── format(ts, fmt?) — format timestamp to string
├── parse(str, fmt?) — parse string to timestamp
├── diff(ts1, ts2) — difference in seconds
├── add(ts, secs)  — add seconds to timestamp
├── year(ts?)      — extract year
├── month(ts?)     — extract month
├── day(ts?)       — extract day
├── hour(ts?)      — extract hour
├── minute(ts?)    — extract minute
└── second(ts?)    — extract second
```
