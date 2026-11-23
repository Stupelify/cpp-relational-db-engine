# C++ Relational Database Engine

Core storage and query layers of a single-user relational database, written in C++. SQL statements are parsed and then executed against paged files on disk through a buffer pool.

This repository contains the components I implemented. It was built with a small team.

## Components

| File | What it does |
|------|--------------|
| `src/buf.cpp` | Buffer pool manager: clock (second-chance) replacement, pin counts, dirty-page write-back, and a hash table from (file, page) to frame |
| `src/heapfile.cpp` | Heap files: header and data pages kept as a linked list of slotted pages; record insert, delete, and fetch by RID; filtered sequential scans (=, <, <=, >, >=, != on int, float, and string attributes) |
| `src/select.cpp` | `SELECT` with projection: looks up attribute descriptors in the system catalog, scans with an optional predicate, and writes projected tuples to the result relation |
| `src/insert.cpp` | `INSERT`: reorders the supplied attributes into catalog order, type-converts values, and builds the fixed-length record |
| `src/delete.cpp` | `DELETE` with an optional predicate, through a heap-file scan |

## How a query flows

```
SQL text -> parser -> query operators (select / insert / delete / join)
                         |
                   system catalog (relation + attribute metadata)
                         |
                   heap files (slotted pages, RIDs)
                         |
                   buffer pool (clock replacement)
                         |
                   database files on disk
```

## Design notes

- Each page is pinned while in use and unpinned with an accurate dirty flag, so the clock algorithm can evict only frames that are both clean and unpinned, or write them back first.
- A scan keeps at most one data page pinned at a time, and follows the next-page links across the file.
- Predicate comparison is type-aware: integers and floats are copied out to aligned locals before comparison, and strings use bounded `strncmp`.

## Tech

C++, Make, Yacc/Lex (SQL parser).
