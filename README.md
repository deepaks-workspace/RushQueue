# CP-015 RushQueue: Fair-Allocation Flash Sale Order Engine

## Team

**Deepak Singh Parihar** — 1AT24IS049  
**Akshay Kumar Ramesh Pujer** — 1AT24IS011

---

## How to run

> Python 3 is required.

```
python main.py
```

The program opens a numbered menu. Navigate by typing the option number and pressing Enter.

---

## Features that work

### Core Features

1. **Flash sale setup** — configure item name, total stock (1–10 000), and per-customer limit (1–100). If the limit is typed larger than the stock, the program silently caps it.
2. **Order placement with unique IDs and arrival sequence** — every order receives an auto-incrementing ID starting at 1001 and a sequential arrival number.
3. **First-come-first-served confirmation** — orders are confirmed in arrival order as long as stock is available.
4. **Waitlist after sell-out** — once available stock hits zero, new orders are queued in arrival order on a FIFO waitlist.
5. **Cancellation with automatic stock release** — cancelling a confirmed order returns its units to the pool and immediately promotes waiting orders (oldest first) that fit the newly freed quantity.
6. **Final report** — option 5 prints four labelled sections (CONFIRMED, WAITLISTED, CANCELLED, REJECTED) with order ID, arrival number, customer name, quantity, and reason for each entry.

### Stretch Features

7. **Duplicate-order prevention** — a second order from the same customer (case-insensitive) is REJECTED if it arrives within 10 seconds of their existing active order.
8. **500-order simulation** — option 6 fires 500 randomised orders against 150 synthetic customers and prints a summary showing how many were confirmed, waitlisted, duplicate-rejected, limit-rejected, units sold, and at which order number stock ran out.

### Additional Quality Features

- **Input validation** — empty strings, out-of-range integers, and non-numeric text are all re-prompted, never crash.
- **Case-insensitive customer names** — `ravi`, `Ravi`, and `RAVI` are treated as the same customer throughout.

---

## Test results

| # | Scenario | Result |
|---|----------|--------|
| a | Options 2, 3, 5, 6 before setup all print "No sale set up yet" | ✅ Pass |
| b | Setup rejects empty name, stock 0, stock "abc", limit 0; then accepts valid inputs | ✅ Pass |
| c | ravi ×2 and asha ×1 → CONFIRMED; kiran ×1, meena ×2, joe ×1 → WAITLISTED | ✅ Pass |
| d | Cancel 1001 (ravi ×2): 2 units released; kiran and joe promoted to CONFIRMED; meena (needs 2, only 0 left after kiran+joe) stays WAITLISTED | ✅ Pass |
| e | Cancelling 1001 again → "already cancelled"; 9999 → "not found"; cancelling waitlisted meena → "Removed from the waitlist" | ✅ Pass |
| f | Same customer within 10 seconds → REJECTED "Duplicate order within 10 seconds" | ✅ Pass |
| g | Same customer (RAVI vs Ravi) after 11 seconds but over limit → REJECTED "Exceeds per-customer limit of 3 (already holding 3)" | ✅ Pass |
| h | Final report shows all four sections with correct reasons | ✅ Pass |
| i | Simulation: confirmed + waitlisted + dup-rejected + limit-rejected = 500; units sold = 100 | ✅ Pass |

---

## Sample run

### 1 — Flash sale setup (with invalid inputs caught)

```
--- Flash Sale Setup ---
Item name:
  Input cannot be empty.
Item name: Phone
Total stock (1-10000):   0
  Value must be between 1 and 10000.
Total stock (1-10000): abc
  Invalid input. Please enter a number.
Total stock (1-10000): 3
Per-customer limit (1-100): 0
  Value must be between 1 and 100.
Per-customer limit (1-100): 3

Sale created: Phone | Stock: 3 | Limit per customer: 3
```

### 2 — Confirmed orders

```
--- Place Order ---
Customer name: ravi
Quantity (1-100): 2

Order ID: 1001 | Arrival #1 | ravi x2
Status: CONFIRMED | Stock left: 1

--- Place Order ---
Customer name: asha
Quantity (1-100): 1

Order ID: 1002 | Arrival #2 | asha x1
Status: CONFIRMED | Stock left: 0
```

### 3 — Waitlisted orders

```
--- Place Order ---
Customer name: kiran
Quantity (1-100): 1

Order ID: 1003 | Arrival #3 | kiran x1
Status: WAITLISTED | Waitlist position: 1

--- Place Order ---
Customer name: meena
Quantity (1-100): 2

Order ID: 1004 | Arrival #4 | meena x2
Status: WAITLISTED | Waitlist position: 2

--- Place Order ---
Customer name: joe
Quantity (1-100): 1

Order ID: 1005 | Arrival #5 | joe x1
Status: WAITLISTED | Waitlist position: 3
```

### 4 — Rejected order (duplicate within 10 s)

```
--- Place Order ---
Customer name: ravi
Quantity (1-100): 1

Order ID: 1006 | Arrival #6 | ravi x1
Status: REJECTED | Reason: Duplicate order within 10 seconds
```

### 5 — Cancellation with waitlist promotion

```
--- Cancel Order ---
Order ID to cancel: 1001
Order 1001 (ravi x2) cancelled.
Released 2 unit(s). Stock now: 2
  -> Waitlisted order 1003 (kiran x1) is now CONFIRMED.
  -> Waitlisted order 1005 (joe x1) is now CONFIRMED.
```

*(meena needs 2 units but only 0 remained after kiran and joe were promoted, so she stays WAITLISTED)*

### 6 — Final report

```
==================================================
             RUSHQUEUE - FINAL REPORT
==================================================
Item             : Phone
Total stock      : 3
Units sold       : 3
Stock remaining  : 0
Per-customer cap : 3
Orders received  : 6

--- CONFIRMED ORDERS ---
OrderID  Arrival  Customer        Qty   Reason
1002     2        asha            1     Stock available
1003     3        kiran           1     Promoted from waitlist after cancellation
1005     5        joe             1     Promoted from waitlist after cancellation
Total: 3 order(s)

--- WAITLISTED ORDERS ---
OrderID  Arrival  Customer        Qty   Reason
1004     4        meena           2     Insufficient stock, added to waitlist
Total: 1 order(s)

--- CANCELLED ORDERS ---
OrderID  Arrival  Customer        Qty   Reason
1001     1        ravi            2     Cancelled by customer
Total: 1 order(s)

--- REJECTED ORDERS ---
OrderID  Arrival  Customer        Qty   Reason
1006     6        ravi            1     Duplicate order within 10 seconds
Total: 1 order(s)

==================================================
```

### 7 — Simulation (stock 100, limit 5, 500 orders)

```
========== SIMULATION RESULT ==========
Item                : Gadget
Starting stock      : 100
Per-customer limit  : 5
Orders simulated    : 500
Confirmed           : 36
Waitlisted          : 194
Rejected (duplicate): 19
Rejected (limit)    : 251
Units sold          : 100
Stock remaining     : 0
Stock lasted        : sold out at order #43 (46 simulated seconds)
=======================================
Choose option 5 to see the full order report.
```
