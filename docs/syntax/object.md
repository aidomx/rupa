# Object

## Apa yang bisa ditulis?

```rupa
person = { name: "Rupa", age: 1 }
```

Nested object:

```rupa
company = { ceo: { name: "Boss", title: "CEO" } }
```

Object dengan array:

```rupa
team = { members: ["Alice", "Bob", "Charlie"] }
```

Array of objects:

```rupa
people = [{ name: "A", score: 90 }, { name: "B", score: 85 }]
```

## Kapan digunakan?

Gunakan object untuk mengelompokkan data dengan key-value pairs. Akses member menggunakan dot notation.

## Apa hasilnya?

Object dibuat dengan `{ }` dan setiap entry berupa `key: value`. Member diakses dengan `.` notation.

### Contoh execution

```rupa
person = { name: "Rupa", age: 1 }
print(person.name)
print(person.age)
```

Output:
```
Rupa
1
```

```rupa
company = { ceo: { name: "Boss", title: "CEO" } }
print(company.ceo.name)
print(company.ceo.title)
```

Output:
```
Boss
CEO
```

```rupa
team = { members: ["Alice", "Bob", "Charlie"] }
print(team.members[0])
print(team.members[2])
```

Output:
```
Alice
Charlie
```

```rupa
people = [{ name: "A", score: 90 }, { name: "B", score: 85 }]
print(people[0].name)
print(people[1].score)
```

Output:
```
A
85
```
