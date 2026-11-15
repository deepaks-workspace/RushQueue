import time
import random

DUP_WINDOW_SEC = 10
MAX_NAME_LEN = 49

# Order statuses
ST_CONFIRMED = "CONFIRMED"
ST_WAITLISTED = "WAITLISTED"
ST_CANCELLED = "CANCELLED"
ST_REJECTED = "REJECTED"

# Global data
sale = {"item": "", "total": 0, "available": 0, "limit": 0, "is_setup": False}
orders = []          # every order, in arrival order (each order is a dictionary)
waitlist = []        # FIFO queue of waitlisted orders
next_order_id = 1001
next_arrival_seq = 1


# ---------- Input helpers ----------

def read_int(prompt, min_val, max_val):
    """Keeps asking until the user types a whole number in the range."""
    while True:
        text = input(prompt).strip()
        try:
            value = int(text)
        except ValueError:
            print("  Invalid input. Please enter a number.")
            continue
        if value < min_val or value > max_val:
            print(f"  Value must be between {min_val} and {max_val}.")
        else:
            return value


def read_string(prompt):
    """Keeps asking until the user types a non-empty text."""
    while True:
        text = input(prompt).strip()
        if text == "":
            print("  Input cannot be empty.")
        else:
            return text[:MAX_NAME_LEN]


# ---------- Flash sale setup ----------

def setup_sale():
    if sale["is_setup"]:
        print(f"\nA sale is already set up for '{sale['item']}'.")
        print("Restart the program to create a new sale.")
        return

    print("\n--- Flash Sale Setup ---")
    sale["item"] = read_string("Item name: ")
    sale["total"] = read_int("Total stock (1-10000): ", 1, 10000)
    sale["limit"] = read_int("Per-customer limit (1-100): ", 1, 100)

    if sale["limit"] > sale["total"]:
        print(f"  Limit cannot exceed total stock. Setting limit = {sale['total']}.")
        sale["limit"] = sale["total"]

    sale["available"] = sale["total"]
    sale["is_setup"] = True

    print(f"\nSale created: {sale['item']} | Stock: {sale['total']} | "
          f"Limit per customer: {sale['limit']}")


def show_sale_info():
    if not sale["is_setup"]:
        print("\nNo sale set up yet. Choose option 1 first.")
        return
    print("\n--- Sale Info ---")
    print(f"Item            : {sale['item']}")
    print(f"Total stock     : {sale['total']}")
    print(f"Available stock : {sale['available']}")
    print(f"Per-customer cap: {sale['limit']}")

# ---------- Order placement ----------

def same_name(a, b):
    """Case-insensitive name comparison."""
    return a.lower() == b.lower()


def customer_active_qty(name):
    """Total quantity a customer holds (confirmed + waitlisted)."""
    total = 0
    for o in orders:
        if same_name(o["customer"], name) and o["status"] in (ST_CONFIRMED, ST_WAITLISTED):
            total += o["qty"]
    return total


def is_duplicate_order(name, now):
    """True if the same customer has a confirmed/waitlisted order
    placed less than DUP_WINDOW_SEC seconds ago."""
    for o in orders:
        if (same_name(o["customer"], name)
                and o["status"] in (ST_CONFIRMED, ST_WAITLISTED)
                and now - o["time"] < DUP_WINDOW_SEC):
            return True
    return False


def process_order(name, qty, now):
    """Core order logic: confirms, waitlists or rejects. Returns the new order."""
    global next_order_id, next_arrival_seq

    order = {"id": next_order_id, "seq": next_arrival_seq, "customer": name,
             "qty": qty, "time": now, "status": "", "reason": ""}
    next_order_id += 1
    next_arrival_seq += 1

    used = customer_active_qty(name)

    if is_duplicate_order(name, now):
        order["status"] = ST_REJECTED
        order["reason"] = f"Duplicate order within {DUP_WINDOW_SEC} seconds"
    elif used + qty > sale["limit"]:
        order["status"] = ST_REJECTED
        order["reason"] = (f"Exceeds per-customer limit of {sale['limit']} "
                           f"(already holding {used})")
    elif qty <= sale["available"]:
        order["status"] = ST_CONFIRMED
        order["reason"] = "Stock available"
        sale["available"] -= qty
    else:
        order["status"] = ST_WAITLISTED
        order["reason"] = "Insufficient stock, added to waitlist"
        waitlist.append(order)

    orders.append(order)
    return order


def place_order():
    if not sale["is_setup"]:
        print("\nNo sale set up yet. Choose option 1 first.")
        return

    print("\n--- Place Order ---")
    name = read_string("Customer name: ")
    qty = read_int("Quantity (1-100): ", 1, 100)

    order = process_order(name, qty, time.time())

    print(f"\nOrder ID: {order['id']} | Arrival #{order['seq']} | "
          f"{order['customer']} x{order['qty']}")
    if order["status"] == ST_CONFIRMED:
        print(f"Status: CONFIRMED | Stock left: {sale['available']}")
    elif order["status"] == ST_WAITLISTED:
        print(f"Status: WAITLISTED | Waitlist position: {len(waitlist)}")
    else:
        print(f"Status: REJECTED | Reason: {order['reason']}")

# ---------- Cancellation and waitlist allocation ----------

def find_order(order_id):
    """Returns the order with this id, or None if it does not exist."""
    for o in orders:
        if o["id"] == order_id:
            return o
    return None


def process_waitlist():
    """Walks the waitlist oldest first and confirms every waiting order that
    fits the available stock. Orders too big for the stock keep their place."""
    for w in waitlist[:]:                 # loop over a copy, because we remove items
        if sale["available"] <= 0:
            break
        if w["qty"] <= sale["available"]:
            w["status"] = ST_CONFIRMED
            w["reason"] = "Promoted from waitlist after cancellation"
            sale["available"] -= w["qty"]
            waitlist.remove(w)
            print(f"  -> Waitlisted order {w['id']} ({w['customer']} x{w['qty']}) "
                  f"is now CONFIRMED.")


def cancel_order():
    if not sale["is_setup"]:
        print("\nNo sale set up yet. Choose option 1 first.")
        return
    if len(orders) == 0:
        print("\nNo orders have been placed yet.")
        return

    print("\n--- Cancel Order ---")
    order_id = read_int("Order ID to cancel: ", 1, 999999)
    order = find_order(order_id)

    if order is None:
        print(f"Order {order_id} not found.")
        return
    if order["status"] == ST_CANCELLED:
        print(f"Order {order_id} is already cancelled.")
        return
    if order["status"] == ST_REJECTED:
        print(f"Order {order_id} was rejected and cannot be cancelled.")
        return

    was_confirmed = (order["status"] == ST_CONFIRMED)
    order["status"] = ST_CANCELLED
    order["reason"] = "Cancelled by customer"
    print(f"Order {order['id']} ({order['customer']} x{order['qty']}) cancelled.")

    if was_confirmed:
        sale["available"] += order["qty"]
        print(f"Released {order['qty']} unit(s). Stock now: {sale['available']}")
        process_waitlist()
    else:
        waitlist.remove(order)
        print("Removed from the waitlist.")

# ---------- Final report ----------

def print_section(status, title):
    print(f"\n--- {title} ---")
    print(f"{'OrderID':<8} {'Arrival':<8} {'Customer':<15} {'Qty':<5} Reason")

    count = 0
    for o in orders:
        if o["status"] == status:
            print(f"{o['id']:<8} {o['seq']:<8} {o['customer']:<15} "
                  f"{o['qty']:<5} {o['reason']}")
            count += 1

    if count == 0:
        print("  (none)")
    print(f"Total: {count} order(s)")


def final_report():
    if not sale["is_setup"]:
        print("\nNo sale set up yet. Choose option 1 first.")
        return

    print("\n==================================================")
    print("             RUSHQUEUE - FINAL REPORT")
    print("==================================================")
    print(f"Item             : {sale['item']}")
    print(f"Total stock      : {sale['total']}")
    print(f"Units sold       : {sale['total'] - sale['available']}")
    print(f"Stock remaining  : {sale['available']}")
    print(f"Per-customer cap : {sale['limit']}")
    print(f"Orders received  : {len(orders)}")

    print_section(ST_CONFIRMED, "CONFIRMED ORDERS")
    print_section(ST_WAITLISTED, "WAITLISTED ORDERS")
    print_section(ST_CANCELLED, "CANCELLED ORDERS")
    print_section(ST_REJECTED, "REJECTED ORDERS")

    print("\n==================================================")

# ---------- Simulation ----------

SIM_ORDERS = 500
SIM_CUSTOMERS = 150


def reset_orders():
    """Clears all orders and refills the stock, so a simulation starts fresh."""
    global next_order_id, next_arrival_seq
    orders.clear()
    waitlist.clear()
    next_order_id = 1001
    next_arrival_seq = 1
    sale["available"] = sale["total"]


def simulate_orders():
    if not sale["is_setup"]:
        print("\nNo sale set up yet. Choose option 1 first.")
        return

    if len(orders) > 0:
        answer = read_int("This will clear all current orders. "
                          "Continue? (1=yes, 0=no): ", 0, 1)
        if answer == 0:
            print("Simulation cancelled.")
            return

    reset_orders()

    clock = 0              # simulated time in seconds
    sold_out_at = 0        # order number at which the stock ran out
    sold_out_time = 0

    for i in range(1, SIM_ORDERS + 1):
        clock += random.randint(0, 2)               # next order arrives 0-2 sec later
        name = f"Cust{random.randint(1, SIM_CUSTOMERS)}"
        qty = random.randint(1, sale["limit"])
        process_order(name, qty, clock)

        if sold_out_at == 0 and sale["available"] == 0:
            sold_out_at = i
            sold_out_time = clock

    confirmed = 0
    waitlisted = 0
    dup_rejected = 0
    limit_rejected = 0
    for o in orders:
        if o["status"] == ST_CONFIRMED:
            confirmed += 1
        elif o["status"] == ST_WAITLISTED:
            waitlisted += 1
        elif o["reason"].startswith("Duplicate"):
            dup_rejected += 1
        else:
            limit_rejected += 1

    print("\n========== SIMULATION RESULT ==========")
    print(f"Item                : {sale['item']}")
    print(f"Starting stock      : {sale['total']}")
    print(f"Per-customer limit  : {sale['limit']}")
    print(f"Orders simulated    : {SIM_ORDERS}")
    print(f"Confirmed           : {confirmed}")
    print(f"Waitlisted          : {waitlisted}")
    print(f"Rejected (duplicate): {dup_rejected}")
    print(f"Rejected (limit)    : {limit_rejected}")
    print(f"Units sold          : {sale['total'] - sale['available']}")
    print(f"Stock remaining     : {sale['available']}")
    if sold_out_at > 0:
        print(f"Stock lasted        : sold out at order #{sold_out_at} "
              f"({sold_out_time} simulated seconds)")
    else:
        print(f"Stock lasted        : did not sell out in {SIM_ORDERS} orders")
    print("=======================================")
    print("Choose option 5 to see the full order report.")

# ---------- Menu ----------

def print_menu():
    print("\n==============================")
    print("   RUSHQUEUE - FLASH SALE")
    print("==============================")
    print("1. Setup flash sale")
    print("2. Place order")
    print("3. Cancel order")
    print("4. Show sale info")
    print("5. Final report")
    print("6. Simulate 500 random orders")
    print("0. Exit")
    print("------------------------------")


def main():
    while True:
        print_menu()
        choice = read_int("Enter choice: ", 0, 6)

        if choice == 1:
            setup_sale()
        elif choice == 2:
            place_order()
        elif choice == 3:
            cancel_order()
        elif choice == 4:
            show_sale_info()
        elif choice == 5:
            final_report()
        elif choice == 6:
            simulate_orders()
        elif choice == 0:
            print("\nGoodbye!")
            break


if __name__ == "__main__":
    try:
        main()
    except (KeyboardInterrupt, EOFError):
        print("\n\nProgram interrupted. Goodbye!")