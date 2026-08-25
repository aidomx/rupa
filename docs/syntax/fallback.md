# Fallback

## Apa yang bisa ditulis?

```rupa
value = first | second
value = primary | fallback | default
```

Dapat digabung dengan `->`:

```rupa
result ?= valid -> primary | fallback | "Unavailable"
```

## Kapan digunakan?

Gunakan `|` ketika beberapa expression disusun sebagai pilihan atau fallback.

## Apa hasilnya?

Expression membentuk chain fallback. `|` dapat digunakan sendiri sebagai bagian dari expression dan tidak bergantung pada `?=` atau `->`.
