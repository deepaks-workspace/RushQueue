# CP-015 RushQueue: Fair-Allocation Flash Sale Order Engine

## Team

Deepak Singh Parihar 1AT24IS049, 
Akshay Kumar Ramesh Pujer 1AT24IS011

## How to run

```
gcc main.c -o rushqueue
./rushqueue
```

On Windows PowerShell, run `.\rushqueue.exe` instead of `./rushqueue`.

## Features that work

Core Features

1. Flash sale setup with item name, total stock and per-customer order limit
2. Order placement with unique order IDs and arrival sequence numbers
3. First-come-first-served confirmation until stock ends
4. Waitlist for orders placed after sell-out
5. Order cancellation that releases stock and automatically confirms the next waitlisted order that fits
6. Final report showing confirmed, waitlisted, cancelled and rejected orders with reasons

Stretch Features

-   Duplicate-order prevention: same customer within 10 seconds is rejected
-   Simulation of 500 random orders with a report of how long the stock lasted

Other

-   Input validation for numbers, empty names and over-long input
-   Customer names are compared ignoring upper and lower case

## Sample run

Run 1st ;-

    ==============================
    RUSHQUEUE - FLASH SALE
    ==============================
    1. Setup flash sale
    2. Place order
    3. Cancel order
    4. Show sale info
    5. Final report
    0. Exit
    ------------------------------
    Enter choice: 1

    --- Flash Sale Setup ---
    Item name: phone
    Total stock (1-10000): 5
    Per-customer limit (1-100): 3

    Sale created: phone | Stock: 5 | Limit per customer: 3

    Enter choice: 2

    --- Place Order ---
    Customer name: ravi
    Quantity (1-100): 1

    Order ID: 1001 | Arrival #1 | ravi x1
    Status: CONFIRMED | Stock left: 4

    Enter choice: 2

    --- Place Order ---
    Customer name: ravi
    Quantity (1-100): 1

    Order ID: 1002 | Arrival #2 | ravi x1
    Status: CONFIRMED | Stock left: 3

    Enter choice: 2

    --- Place Order ---
    Customer name: ravi
    Quantity (1-100): 1

    Order ID: 1003 | Arrival #3 | ravi x1
    Status: CONFIRMED | Stock left: 2

    Enter choice: 2

    --- Place Order ---
    Customer name: asha
    Quantity (1-100): 1

    Order ID: 1004 | Arrival #4 | asha x1
    Status: CONFIRMED | Stock left: 1

    Enter choice: 2

    --- Place Order ---
    Customer name: asha
    Quantity (1-100): 1

    Order ID: 1005 | Arrival #5 | asha x1
    Status: REJECTED | Reason: Duplicate order within 10 seconds

    Enter choice: 0

    Goodbye!

Run 2 (canceling order):-

    --- Place Order ---
    Customer name: ravi
    Quantity (1-100): 2

    Order ID: 1001 | Arrival #1 | ravi x2
    Status: CONFIRMED | Stock left: 1

    --- Cancel Order ---
    Order ID to cancel: 1001
    Order 1001 (ravi x2) cancelled.
    Released 2 unit(s). Stock now: 2
    -> Waitlisted order 1002 (asha x2) is now CONFIRMED.

    --- Cancel Order ---
    Order ID to cancel: 1001
    Order 1001 (ravi  x1) cancelled.
    Removed from the waitlist.

Run 3 :-
Enter choice: 5

    ==================================================
                RUSHQUEUE - FINAL REPORT
    ==================================================
    Item             : phone
    Total stock      : 3
    Units sold       : 2
    Stock remaining  : 1
    Per-customer cap : 3
    Orders received  : 5

    --- CONFIRMED ORDERS ---
    OrderID  Arrival  Customer        Qty   Reason
    1002     2        asha            1     Stock available
    1003     3        kiran           1     Promoted from waitlist after cancellation
    Total: 2 order(s)

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
    1005     5        kiran           3     Exceeds per-customer limit of 3 (already holding 1)
    Total: 1 order(s)

    ==================================================

Run 4 :-

    Enter choice: 1

    --- Flash Sale Setup ---
    Item name: phone
    Total stock (1-10000): 100
    Per-customer limit (1-100): 5

    Sale created: phone | Stock: 100 | Limit per customer: 5

    ==============================
    RUSHQUEUE - FLASH SALE
    ==============================
    1. Setup flash sale
    2. Place order
    3. Cancel order
    4. Show sale info
    5. Final report
    6. Simulate 500 random orders
    0. Exit
    ------------------------------
    Enter choice: 6

    ========== SIMULATION RESULT ==========
    Item                : phone
    Starting stock      : 100
    Per-customer limit  : 5
    Orders simulated    : 500
    Confirmed           : 32
    Waitlisted          : 201
    Rejected (duplicate): 16
    Rejected (limit)    : 251
    Units sold          : 100
    Stock remaining     : 0
    Stock lasted        : sold out at order #34 (36 simulated seconds)
    =======================================
    Choose option 5 to see the full order report.

Run 5:-

    Enter choice: 1

    --- Flash Sale Setup ---
    Item name: phone
    Total stock (1-10000): 5
    Per-customer limit (1-100): 3

    Sale created: phone | Stock: 5 | Limit per customer: 3

    Enter choice: 2

    --- Place Order ---
    Customer name: ravi
    Quantity (1-100): 2

    Order ID: 1001 | Arrival #1 | ravi x2
    Status: CONFIRMED | Stock left: 3

    Enter choice: 2

    --- Place Order ---
    Customer name: RAVI
    Quantity (1-100): 2

    Order ID: 1002 | Arrival #2 | RAVI x2
    Status: REJECTED | Reason: Exceeds per-customer limit of 3 (already holding 2)

    Enter choice: 2

    --- Place Order ---
    Customer name: raVi
    Quantity (1-100): 2

    Order ID: 1003 | Arrival #3 | raVi x2
    Status: REJECTED | Reason: Exceeds per-customer limit of 3 (already holding 2)

    Enter choice: 2

    --- Place Order ---
    Customer name: gdayiadwefkhwfbvwakncgngniangncncynggyrcgyncayugugy
    Quantity (1-100): 1

    Order ID: 1004 | Arrival #4 | gdayiadwefkhwfbvwakncgngniangncncynggyrcgyncayugu x1
    Status: CONFIRMED | Stock left: 2
