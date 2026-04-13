# SwiftServe – Multithreaded HTTP Server in C++

A high-performance HTTP server built from scratch in C++ using TCP sockets, multithreading, and a thread pool.

---

## Overview

SwiftServe is a lightweight web server that demonstrates low-level networking, concurrency, and HTTP protocol handling without relying on external frameworks.

---

## Features

- TCP Socket Programming (POSIX)
- Multithreaded architecture with thread pool
- Request parsing (HTTP/1.1)
- Routing support (`/`, `/about`, `/api`)
- Static file serving (HTML, CSS, JS)
- MIME type detection
- Content-Length handling (HTTP compliant)
- Thread-safe logging system

---

## Project Structure
SwiftServe/
│
├── server.cpp
├── logs.txt
└── public/
├── index.html
├── about.html
└── style.css


---

## Getting Started

### 1. Compile

```bash
g++ server.cpp -o server -pthread
```
### 2. Run

```bash
./server
```
### 3. Open browser
http://localhost:8080