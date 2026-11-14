#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_ORDERS 1000
#define NAME_LEN 50
#define REASON_LEN 100
#define DUP_WINDOW_SEC 10

/* Order statuses */
#define ST_CONFIRMED 1
#define ST_WAITLISTED 2
#define ST_CANCELLED 3
#define ST_REJECTED 4

typedef struct
{
    int orderId;
    int arrivalSeq;
    char customer[NAME_LEN];
    int quantity;
    long timestamp; /* seconds, used for duplicate check */
    int status;     /* ST_* constants */
    char reason[REASON_LEN];
} Order;

typedef struct
{
    char itemName[NAME_LEN];
    int totalStock;
    int availableStock;
    int perCustomerLimit;
    int isSetup; /* 0 = not set up, 1 = set up */
} Sale;

/* Global data */
Order orders[MAX_ORDERS];
int orderCount = 0;
int nextOrderId = 1001;
int nextArrivalSeq = 1;
Sale sale;

/* Waitlist queue: stores indexes into orders[] (FIFO) */
int waitlist[MAX_ORDERS];
int wlFront = 0;
int wlRear = 0;

/* ---------- Input helpers ---------- */

void clearBuffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

/* Reads an integer between min and max; keeps asking until valid */
/* Reads an integer between min and max; keeps asking until valid */
int readInt(const char *prompt, int min, int max)
{
    char line[100];
    char *end;
    long val;

    while (1)
    {
        printf("%s", prompt);
        if (fgets(line, sizeof(line), stdin) == NULL)
        {
            return min;
        }
        if (strchr(line, '\n') == NULL)
        {
            clearBuffer(); /* discard the rest of a too-long line */
        }
        val = strtol(line, &end, 10);
        if (end == line || (*end != '\n' && *end != '\0'))
        {
            printf("  Invalid input. Please enter a number.\n");
        }
        else if (val < min || val > max)
        {
            printf("  Value must be between %d and %d.\n", min, max);
        }
        else
        {
            return (int)val;
        }
    }
}

/* Reads a non-empty string (no leading/trailing newline) */
void readString(const char *prompt, char *dest, int size)
{
    while (1)
    {
        printf("%s", prompt);
        if (fgets(dest, size, stdin) == NULL)
        {
            dest[0] = '\0';
            return;
        }
        if (strchr(dest, '\n') == NULL)
        {
            clearBuffer(); /* discard the rest of a too-long line */
        }
        dest[strcspn(dest, "\n")] = '\0';
        if (strlen(dest) == 0)
        {
            printf("  Input cannot be empty.\n");
        }
        else
        {
            return;
        }
    }
}

/* ---------- Flash sale setup ---------- */

void setupSale()
{
    if (sale.isSetup)
    {
        printf("\nA sale is already set up for '%s'.\n", sale.itemName);
        printf("Restart the program to create a new sale.\n");
        return;
    }

    printf("\n--- Flash Sale Setup ---\n");
    readString("Item name: ", sale.itemName, NAME_LEN);
    sale.totalStock = readInt("Total stock (1-10000): ", 1, 10000);
    sale.perCustomerLimit = readInt("Per-customer limit (1-100): ", 1, 100);

    if (sale.perCustomerLimit > sale.totalStock)
    {
        printf("  Limit cannot exceed total stock. Setting limit = %d.\n", sale.totalStock);
        sale.perCustomerLimit = sale.totalStock;
    }

    sale.availableStock = sale.totalStock;
    sale.isSetup = 1;

    printf("\nSale created: %s | Stock: %d | Limit per customer: %d\n",
           sale.itemName, sale.totalStock, sale.perCustomerLimit);
}

void showSaleInfo()
{
    if (!sale.isSetup)
    {
        printf("\nNo sale set up yet. Choose option 1 first.\n");
        return;
    }
    printf("\n--- Sale Info ---\n");
    printf("Item            : %s\n", sale.itemName);
    printf("Total stock     : %d\n", sale.totalStock);
    printf("Available stock : %d\n", sale.availableStock);
    printf("Per-customer cap: %d\n", sale.perCustomerLimit);
}

/* ---------- Order placement ---------- */

/* Case-insensitive name comparison: returns 1 if the names match */
int sameName(const char *a, const char *b)
{
    while (*a && *b)
    {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
        {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

/* Total quantity a customer currently holds (confirmed + waitlisted) */
int customerActiveQty(const char *name)
{
    int i, total = 0;
    for (i = 0; i < orderCount; i++)
    {
        if (sameName(orders[i].customer, name) &&
            (orders[i].status == ST_CONFIRMED || orders[i].status == ST_WAITLISTED))
        {
            total += orders[i].quantity;
        }
    }
    return total;
}

void enqueueWaitlist(int idx)
{
    waitlist[wlRear++] = idx;
}

int findOrderIndex(int orderId)
{
    int i;
    for (i = 0; i < orderCount; i++)
    {
        if (orders[i].orderId == orderId)
        {
            return i;
        }
    }
    return -1;
}

/* Number of orders still waiting in the queue */
int waitlistSize()
{
    int i, n = 0;
    for (i = wlFront; i < wlRear; i++)
    {
        if (orders[waitlist[i]].status == ST_WAITLISTED)
        {
            n++;
        }
    }
    return n;
}

/* Returns 1 if this customer already has a confirmed/waitlisted order
   placed less than DUP_WINDOW_SEC seconds ago */
int isDuplicateOrder(const char *name, long now)
{
    int i;
    for (i = 0; i < orderCount; i++)
    {
        if (sameName(orders[i].customer, name) &&
            (orders[i].status == ST_CONFIRMED || orders[i].status == ST_WAITLISTED) &&
            (now - orders[i].timestamp) < DUP_WINDOW_SEC)
        {
            return 1;
        }
    }
    return 0;
}

/* Core order logic: validates, then confirms, waitlists or rejects.
   Returns the index of the new order in orders[]. */
int processOrder(const char *name, int qty, long now)
{
    Order *o;
    int used;

    o = &orders[orderCount];
    o->orderId = nextOrderId++;
    o->arrivalSeq = nextArrivalSeq++;
    strcpy(o->customer, name);
    o->quantity = qty;
    o->timestamp = now;
    o->reason[0] = '\0';

    used = customerActiveQty(name);

    if (isDuplicateOrder(name, now))
    {
        o->status = ST_REJECTED;
        snprintf(o->reason, REASON_LEN,
                 "Duplicate order within %d seconds", DUP_WINDOW_SEC);
    }
    else if (used + qty > sale.perCustomerLimit)
    {
        o->status = ST_REJECTED;
        snprintf(o->reason, REASON_LEN,
                 "Exceeds per-customer limit of %d (already holding %d)",
                 sale.perCustomerLimit, used);
    }
    else if (qty <= sale.availableStock)
    {
        o->status = ST_CONFIRMED;
        sale.availableStock -= qty;
        strcpy(o->reason, "Stock available");
    }
    else
    {
        o->status = ST_WAITLISTED;
        strcpy(o->reason, "Insufficient stock, added to waitlist");
        enqueueWaitlist(orderCount);
    }

    orderCount++;
    return orderCount - 1;
}

void placeOrder()
{
    Order *o;
    char name[NAME_LEN];
    int qty;

    if (!sale.isSetup)
    {
        printf("\nNo sale set up yet. Choose option 1 first.\n");
        return;
    }
    if (orderCount >= MAX_ORDERS)
    {
        printf("\nOrder capacity reached (%d). Cannot accept more orders.\n", MAX_ORDERS);
        return;
    }

    printf("\n--- Place Order ---\n");
    readString("Customer name: ", name, NAME_LEN);
    qty = readInt("Quantity (1-100): ", 1, 100);

    o = &orders[processOrder(name, qty, (long)time(NULL))];

    printf("\nOrder ID: %d | Arrival #%d | %s x%d\n",
           o->orderId, o->arrivalSeq, o->customer, o->quantity);
    if (o->status == ST_CONFIRMED)
        printf("Status: CONFIRMED | Stock left: %d\n", sale.availableStock);
    else if (o->status == ST_WAITLISTED)
        printf("Status: WAITLISTED | Waitlist position: %d\n", waitlistSize());
    else
        printf("Status: REJECTED | Reason: %s\n", o->reason);
}

/* ---------- Cancellation and waitlist allocation ---------- */

/* Walks the waitlist in arrival order and confirms every waiting order
   that fits the available stock. Orders too big for the current stock
   keep their place in the queue. */
void processWaitlist()
{
    int i;
    Order *w;

    for (i = wlFront; i < wlRear && sale.availableStock > 0; i++)
    {
        w = &orders[waitlist[i]];
        if (w->status == ST_WAITLISTED && w->quantity <= sale.availableStock)
        {
            w->status = ST_CONFIRMED;
            sale.availableStock -= w->quantity;
            strcpy(w->reason, "Promoted from waitlist after cancellation");
            printf("  -> Waitlisted order %d (%s x%d) is now CONFIRMED.\n",
                   w->orderId, w->customer, w->quantity);
        }
    }

    /* Move the front past orders that are no longer waiting */
    while (wlFront < wlRear && orders[waitlist[wlFront]].status != ST_WAITLISTED)
    {
        wlFront++;
    }
}

void cancelOrder()
{
    int id, idx, wasConfirmed;

    if (!sale.isSetup)
    {
        printf("\nNo sale set up yet. Choose option 1 first.\n");
        return;
    }
    if (orderCount == 0)
    {
        printf("\nNo orders have been placed yet.\n");
        return;
    }

    printf("\n--- Cancel Order ---\n");
    id = readInt("Order ID to cancel: ", 1, 999999);
    idx = findOrderIndex(id);

    if (idx == -1)
    {
        printf("Order %d not found.\n", id);
        return;
    }
    if (orders[idx].status == ST_CANCELLED)
    {
        printf("Order %d is already cancelled.\n", id);
        return;
    }
    if (orders[idx].status == ST_REJECTED)
    {
        printf("Order %d was rejected and cannot be cancelled.\n", id);
        return;
    }

    wasConfirmed = (orders[idx].status == ST_CONFIRMED);
    orders[idx].status = ST_CANCELLED;
    strcpy(orders[idx].reason, "Cancelled by customer");

    printf("Order %d (%s x%d) cancelled.\n",
           orders[idx].orderId, orders[idx].customer, orders[idx].quantity);

    if (wasConfirmed)
    {
        sale.availableStock += orders[idx].quantity;
        printf("Released %d unit(s). Stock now: %d\n",
               orders[idx].quantity, sale.availableStock);
        processWaitlist();
    }
    else
    {
        printf("Removed from the waitlist.\n");
    }
}

/* ---------- Final report ---------- */

void printSection(int status, const char *title)
{
    int i, count = 0;

    printf("\n--- %s ---\n", title);
    printf("%-8s %-8s %-15s %-5s %s\n",
           "OrderID", "Arrival", "Customer", "Qty", "Reason");

    for (i = 0; i < orderCount; i++)
    {
        if (orders[i].status == status)
        {
            printf("%-8d %-8d %-15s %-5d %s\n",
                   orders[i].orderId, orders[i].arrivalSeq,
                   orders[i].customer, orders[i].quantity,
                   orders[i].reason);
            count++;
        }
    }
    if (count == 0)
    {
        printf("  (none)\n");
    }
    printf("Total: %d order(s)\n", count);
}

void finalReport()
{
    if (!sale.isSetup)
    {
        printf("\nNo sale set up yet. Choose option 1 first.\n");
        return;
    }

    printf("\n==================================================\n");
    printf("             RUSHQUEUE - FINAL REPORT\n");
    printf("==================================================\n");
    printf("Item             : %s\n", sale.itemName);
    printf("Total stock      : %d\n", sale.totalStock);
    printf("Units sold       : %d\n", sale.totalStock - sale.availableStock);
    printf("Stock remaining  : %d\n", sale.availableStock);
    printf("Per-customer cap : %d\n", sale.perCustomerLimit);
    printf("Orders received  : %d\n", orderCount);

    printSection(ST_CONFIRMED, "CONFIRMED ORDERS");
    printSection(ST_WAITLISTED, "WAITLISTED ORDERS");
    printSection(ST_CANCELLED, "CANCELLED ORDERS");
    printSection(ST_REJECTED, "REJECTED ORDERS");

    printf("\n==================================================\n");
}

/* ---------- Simulation ---------- */

#define SIM_ORDERS 500
#define SIM_CUSTOMERS 150

void resetOrders()
{
    orderCount = 0;
    nextOrderId = 1001;
    nextArrivalSeq = 1;
    wlFront = 0;
    wlRear = 0;
    sale.availableStock = sale.totalStock;
}

void simulateOrders()
{
    int i, confirmed = 0, waitlisted = 0, dupRej = 0, limitRej = 0;
    int soldOutAt = 0;
    long t = 1000, startT = 1000, soldOutTime = 0;
    char name[NAME_LEN];

    if (!sale.isSetup)
    {
        printf("\nNo sale set up yet. Choose option 1 first.\n");
        return;
    }
    if (orderCount > 0)
    {
        if (readInt("This will clear all current orders. Continue? (1=yes, 0=no): ", 0, 1) == 0)
        {
            printf("Simulation cancelled.\n");
            return;
        }
    }

    resetOrders();
    srand((unsigned)time(NULL));

    for (i = 0; i < SIM_ORDERS; i++)
    {
        t += rand() % 3; /* next order arrives 0-2 sec later */
        snprintf(name, NAME_LEN, "Cust%d", rand() % SIM_CUSTOMERS + 1);
        processOrder(name, rand() % sale.perCustomerLimit + 1, t);

        if (soldOutAt == 0 && sale.availableStock == 0)
        {
            soldOutAt = i + 1;
            soldOutTime = t - startT;
        }
    }

    for (i = 0; i < orderCount; i++)
    {
        if (orders[i].status == ST_CONFIRMED)
            confirmed++;
        else if (orders[i].status == ST_WAITLISTED)
            waitlisted++;
        else if (strncmp(orders[i].reason, "Duplicate", 9) == 0)
            dupRej++;
        else
            limitRej++;
    }

    printf("\n========== SIMULATION RESULT ==========\n");
    printf("Item                : %s\n", sale.itemName);
    printf("Starting stock      : %d\n", sale.totalStock);
    printf("Per-customer limit  : %d\n", sale.perCustomerLimit);
    printf("Orders simulated    : %d\n", SIM_ORDERS);
    printf("Confirmed           : %d\n", confirmed);
    printf("Waitlisted          : %d\n", waitlisted);
    printf("Rejected (duplicate): %d\n", dupRej);
    printf("Rejected (limit)    : %d\n", limitRej);
    printf("Units sold          : %d\n", sale.totalStock - sale.availableStock);
    printf("Stock remaining     : %d\n", sale.availableStock);
    if (soldOutAt > 0)
        printf("Stock lasted        : sold out at order #%d (%ld simulated seconds)\n",
               soldOutAt, soldOutTime);
    else
        printf("Stock lasted        : did not sell out in %d orders\n", SIM_ORDERS);
    printf("=======================================\n");
    printf("Choose option 5 to see the full order report.\n");
}

/* ---------- Menu ---------- */

void printMenu()
{
    printf("\n==============================\n");
    printf("   RUSHQUEUE - FLASH SALE\n");
    printf("==============================\n");
    printf("1. Setup flash sale\n");
    printf("2. Place order\n");
    printf("3. Cancel order\n");
    printf("4. Show sale info\n");
    printf("5. Final report\n");
    printf("6. Simulate 500 random orders\n");
    printf("0. Exit\n");
    printf("------------------------------\n");
}

int main()
{
    int choice;
    memset(&sale, 0, sizeof(sale));

    do
    {
        printMenu();
        choice = readInt("Enter choice: ", 0, 6);

        switch (choice)
        {
        case 1:
            setupSale();
            break;
        case 2:
            placeOrder();
            break;
        case 3:
            cancelOrder();
            break;
        case 4:
            showSaleInfo();
            break;
        case 5:
            finalReport();
            break;
        case 6:
            simulateOrders();
            break;
        case 0:
            printf("\nGoodbye!\n");
            break;
        }
    } while (choice != 0);

    return 0;
}