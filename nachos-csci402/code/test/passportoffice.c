

/* passportoffice.c
 *      Passport office as a user program
 */

#include "syscall.h"

#define CLERK_NUMBER 20
#define CUSTOMER_NUMBER 60
#define CLERK_TYPES 4
#define SENATOR_NUMBER 10
#define false 0
#define true 1

void ApplicationClerk(); /* used to have int myLine in parameter */
void PictureClerk();
void PassportClerk();
void Cashier();
void Customer(); /* used to have int custNumber in parameter */
void Senator();
void Manager();

struct CustomerAttribute {
    int SSN;
    int likesPicture;
    int applicationIsFiled;
    int hasCertification;
    int isDone;
    int clerkMessedUp;
    int money;
    int currentLine;
};

int globalThreadCount = 0;
int globalClerkCount = 0;
int globalCustCount = 0;
char* threadNames[250];
int currentThread = 0;
int testChosen = 1; /* CL: indicate test number (1-7) or full program (0)*/
int clerkCount = 0;  /* CL: number of total clerks of the 4 types that can be modified*/
int customerCount = 0; /* CL: number of customers that can be modified*/
int senatorCount = 0; /* CL: number of senators that can be modified*/
int senatorLineCount = 0; /* CL: number of senators in a line at any given time*/
int senatorDone = false;

int prevTotalBoolCount = 0;
int currentTotalBoolCount = 0;

/* initialize locks and arrays for linecount, clerk, customer, manager, senator information*/
int clerkIdMap[CLERK_NUMBER];
int clerkLineLock;
int clerkLineCount[CLERK_NUMBER]; /* CL: number of customers in a clerk's regular line*/
int clerkBribeLineCount[CLERK_NUMBER]; /* CL: number of customers in a clerk's bribe line*/
typedef enum {AVAILABLE, BUSY, ONBREAK} ClerkState; /* CL: enum for clerk's conditions*/
ClerkState clerkStates[CLERK_NUMBER] = {AVAILABLE}; /*state of each clerk is available in the beginning*/
int clerkLineCV[CLERK_NUMBER];
int clerkBribeLineCV[CLERK_NUMBER];
int clerkSenatorLineCV;
int clerkSenatorCVLock[CLERK_NUMBER];

int outsideLineCV;
int outsideLock;
int outsideLineCount = 0; /* CL: an outside line for customers to line up if senator is here, or other rare situations*/

int breakCV[CLERK_NUMBER];
int senatorLock;
/* Condition* senatorCV = new Condition("SenatorCV");*/

struct CustomerAttribute customerAttributes[CUSTOMER_NUMBER]; /* CL: customer attributes, accessed by custNumber*/
int customerIdMap[CUSTOMER_NUMBER];
int clerkMoney[CLERK_NUMBER] = {0}; /* CL: every clerk has no bribe money in the beginning*/
/*Senator control variables*/
int senatorLineCV;
int clerkSenatorCV[CLERK_NUMBER];
/*Second monitor*/
int clerkLock[CLERK_NUMBER];
int clerkCV[CLERK_NUMBER];
int customerData[CLERK_NUMBER]; /*HUNG: Every clerk will use this to get the customer's customerAttribute index*/
char* clerkTypesStatic[CLERK_TYPES] = { "ApplicationClerks : ", "PictureClerks     : ", "PassportClerks    : ", "Cashiers          : " };
char* clerkTypes[CLERK_NUMBER];
int clerkArray[CLERK_TYPES];

int breakLock[CLERK_NUMBER];

typedef void (*VoidFunctionPtr)(int arg);

struct CustomerAttribute initCustAttr(int ssn) {
    int moneyArray[4] = {100, 600, 1100, 1600};
    int randomIndex = Rand(4, 0); /*0 to 3*/

    struct CustomerAttribute ca;
    ca.SSN = ssn;
    ca.applicationIsFiled = false;
    ca.likesPicture = false;
    ca.hasCertification = false;
    ca.isDone = false;
    ca.clerkMessedUp = false;

    ca.money = moneyArray[randomIndex];
    return ca;
}


char* reverse_str(char* string) {
    int size = 0, i = 0, temp;
    while(*string != '\0') {
        size++;
    }

    for(i = 0; i < size / 2; ++i) {
        temp = string[i];
        string[i] = string[size - i - 1];
        string[size - i - 1] = temp;
    }

    return string;
}

char* int_to_str(int num) {
    char* numStr;
    int size = 0;
    int smallNum;

    while(num != 0) {
        smallNum = num % 10;
        *numStr = (char)(smallNum + '0');
        num /= 10;
        numStr++;
    }

    return reverse_str(numStr);
}

char* my_strcat(char *dest, const char *src) {
    char *rdest = dest;

    while (*dest)
      dest++;
    while (*dest++ = *src++)
      ;
    return rdest;
}

char* my_strcpy(char *s1, const char *s2) {
    char *s = s1;
    while ((*s++ = *s2++) != 0)
        ;
    return (s1);
}

char* concatClerkNameWithNumber(char* clerkName, int clerkNumber) {
    char* clerkNumStr = int_to_str(clerkNumber);
    return my_strcat(my_strcat(clerkName, "_"), clerkNumStr);
}

char* concatStringWithNumber(char* str, int num) {
    char* numStr = int_to_str(num);
    return my_strcat(str, numStr);
}

void writeWithSize(char* string) {
    int size = 0;
    while(string[size] != '\0') {
        ++size;
    }
    Write(string, size, ConsoleOutput);
}

char* currentThreadGetName(int currentThread) {
    return threadNames[currentThread];
}

/* CL: parameter: an int array that contains numbers of each clerk type
     Summary: gets input from user or test, initialize and print out clerk numbers
     return value: void */

void clerkFactory(int countOfEachClerkType[]) {
    int tempClerkCount = 0, i = 0;
    for(i = 0; i < CLERK_TYPES; ++i) {
        if(testChosen == 0) { /* gets input from user if running full program, does not get input if test */
            do {
                writeWithSize(clerkTypesStatic[i]);
                tempClerkCount = 4;  /* Technically would read user input */
                if(tempClerkCount <= 0 || tempClerkCount > 5) {
                    writeWithSize("    The number of clerks must be between 1 and 5 inclusive!\n");
                }
            } while(tempClerkCount <= 0 || tempClerkCount > 5);
            clerkCount += tempClerkCount;
            clerkArray[i] = tempClerkCount;
        } else {
            clerkArray[i] = countOfEachClerkType[i];
        }
    }
    /* CL: print statements */
    writeWithSize(my_strcat(concatStringWithNumber("Number of ApplicationClerks = ", clerkArray[0]), "\n"));
    writeWithSize(my_strcat(concatStringWithNumber("Number of PictureClerks = ", clerkArray[1]), "\n"));
    writeWithSize(my_strcat(concatStringWithNumber("Number of PassportClerks = ", clerkArray[2]), "\n"));
    writeWithSize(my_strcat(concatStringWithNumber("Number of CashiersClerks = ", clerkArray[3]), "\n"));
    writeWithSize(my_strcat(concatStringWithNumber("Number of Senators = ", senatorCount), "\n"));
}

void createClerkThread(int type, int* clerkNumber) {
    char clerkType[50], clerkName[50];
    int i, temp;
    for(i = 0; i < clerkArray[type]; ++i) {
        if(type == 0) {
            my_strcpy(clerkType, "ApplicationClerk");
            Fork((VoidFunctionPtr)ApplicationClerk);
        } else if(type == 1) {
            my_strcpy(clerkType, "PictureClerk");
            Fork((VoidFunctionPtr)PictureClerk);
        } else if(type == 2) {
            my_strcpy(clerkType, "PassportClerk");
            Fork((VoidFunctionPtr)PassportClerk);
        } else if(type == 3) {
            my_strcpy(clerkType, "Cashier");
            Fork((VoidFunctionPtr)Cashier);
        } else {
            return;
        }

        clerkTypes[*clerkNumber] = clerkType;
        *clerkNumber += 1;
    }
}

void createClerkThreads() {
    int type;
    int clerkNumber;
    for(type = 0; type < 4; ++type) {
        createClerkThread(type, &clerkNumber);
    }
}

void createClerkLocksAndConditions() {
    int i;
    char* name;
    for(i = 0; i < clerkCount; ++i) {
        my_strcpy(name, concatClerkNameWithNumber("ClerkLock", i));
        clerkLock[i] = CreateLock(name);

        my_strcpy(name, concatClerkNameWithNumber("ClerkCV", i));
        clerkCV[i] = CreateCondition(name);

        my_strcpy(name, concatClerkNameWithNumber("ClerkLineCV", i));
        clerkLineCV[i] = CreateCondition(name);

        my_strcpy(name, concatClerkNameWithNumber("ClerkBribeLineCV", i));
        clerkBribeLineCV[i] = CreateCondition(name);

        my_strcpy(name, concatClerkNameWithNumber("BreakLock", i));
        breakLock[i] = CreateLock(name);

        my_strcpy(name, concatClerkNameWithNumber("BreakCV", i));
        breakCV[i] = CreateCondition(name);

        my_strcpy(name, concatClerkNameWithNumber("ClerkSenatorCV", i));
        clerkSenatorCV[i] = CreateCondition(name);

        my_strcpy(name, concatClerkNameWithNumber("ClerkSenatorCV", i));
        clerkSenatorCVLock[i] = CreateLock(name);
    }
}

/*CL: Parameter: Thread*
    Summary: create and fire off customer threads with designated names
    Return value: void*/
void createCustomerThreads() {
    int i;
    for(i = 0; i < customerCount; i++){
        Fork((VoidFunctionPtr)Customer);
    }
}

/*CL: Parameter: Thread*
    Summary: create and fire off senator threads with designated names
    Return value: void*/
void createSenatorThreads(){
    int i;
    for(i = 0; i < senatorCount; i++){
        Fork((VoidFunctionPtr)Senator);
    }
}

/*CL: Parameter: Thread*, int []
    Summary: group all create thread/ lock/ condition functions together and fire off manager
    Return value: void*/
void createTestVariables(int countOfEachClerkType[]) {
    clerkFactory(countOfEachClerkType);
    createClerkLocksAndConditions();
    createClerkThreads();
    createCustomerThreads();
    createSenatorThreads();
    Fork((VoidFunctionPtr)Manager);
}

void Part2() {
    char *name;
    int countOfEachClerkType[CLERK_TYPES] = {0,0,0,0};

    writeWithSize("Starting Part 2\n");
    writeWithSize("Test to run (put 0 for full program): ");
    testChosen = 1; /* technically cin >> */

    if(testChosen == 1) {
        writeWithSize("Starting Test 1\n"); /*Customers always take the shortest line, but no 2 customers ever choose the same shortest line at the same time*/
        customerCount = 10;
        clerkCount = 6;
        senatorCount = 0;
        countOfEachClerkType[0] = 2; countOfEachClerkType[1] = 2; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 2) {
        writeWithSize("Starting Test 2\n"); /*Managers only read one from one Clerk's total money received, at a time*/
        customerCount = 5;
        clerkCount = 4;
        senatorCount = 0;
        countOfEachClerkType[0] = 1; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 3) {
        writeWithSize("Starting Test 3\n"); /*Customers do not leave until they are given their passport by the Cashier.
                                     The Cashier does not start on another customer until they know that the last Customer has left their area*/
        customerCount = 5;
        clerkCount = 4;
        senatorCount = 0;
        countOfEachClerkType[0] = 1; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 4) {
        writeWithSize("Starting Test 4\n"); /*Clerks go on break when they have no one waiting in their line*/
        customerCount = 5;
        clerkCount = 4;
        senatorCount = 0;
        countOfEachClerkType[0] = 1; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 5) {
        writeWithSize("Starting Test 5\n"); /*Managers get Clerks off their break when lines get too long*/
        customerCount = 7;
        clerkCount = 4;
        senatorCount = 0;
        countOfEachClerkType[0] = 1; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 6) {
        writeWithSize("Starting Test 6\n"); /*Total sales never suffers from a race condition*/
        customerCount = 25;
        clerkCount = 4;
        senatorCount = 0;
        countOfEachClerkType[0] = 1; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 7) {
        writeWithSize("Starting Test 7\n"); /*Total sales never suffers from a race condition*/
        customerCount = 7;
        clerkCount = 4;
        senatorCount = 1;
        countOfEachClerkType[0] = 1; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 0) {
        do {
            writeWithSize("Number of customers: ");
            customerCount = 20; /* technically cin >> */
            if(customerCount <= 0 || customerCount > 50) {
                writeWithSize("    The number of customers must be between 1 and 50 inclusive!\n");
            }
        } while(customerCount <= 0 || customerCount > 50);

        do {
            writeWithSize("Number of Senators: ");
            senatorCount = 1; /* technically cin >> */
            if(senatorCount < 0 || senatorCount > 10) {
                writeWithSize("    The number of senators must be between 1 and 10 inclusive!\n");
            }
        } while(senatorCount < 0 || senatorCount > 10);

        createTestVariables(countOfEachClerkType);
    }
}

/* CL: Parameter: int myLine (line number of current clerk)
    Summary: chooses customer from the line, or decides if clerks go on break
    Return value: int (the customer number) */

int chooseCustomerFromLine(int myLine) {
    int testFlag = false;
    do {
        testFlag = false;
        /* TODO: -1 used to be NULL.  Hung needs to figure this out */
        if((senatorLineCount > 0 && clerkSenatorCVLock[myLine] != -1) || (senatorLineCount > 0 && senatorDone)) {
            /* CL: chooses senator line first */

            Acquire(clerkSenatorCVLock[myLine]);
            Signal(clerkSenatorCV[myLine], clerkSenatorCVLock[myLine]);
            /*Wait for senator here, if they need me*/
            Wait(clerkSenatorCV[myLine], clerkSenatorCVLock[myLine]);

            if(senatorLineCount == 0){
              Acquire(clerkLineLock);
              clerkStates[myLine] = AVAILABLE;
              Release(clerkSenatorCVLock[myLine]);
            }else if(senatorLineCount > 0 && senatorDone){
              clerkStates[myLine] = AVAILABLE;
              Release(clerkSenatorCVLock[myLine]);

            }else{
              clerkStates[myLine] = BUSY;
              testFlag = true;
            }
        }else{
            Acquire(clerkLineLock);
            if(clerkBribeLineCount[myLine] > 0) {
                Signal(clerkBribeLineCV[myLine], clerkLineLock);
                clerkStates[myLine] = BUSY; /*redundant setting*/
            } else if(clerkLineCount[myLine] > 0) {
                writeWithSize(my_strcat(currentThreadGetName(currentThread), " is servicing a customer from regular line\n"));
                Signal(clerkLineCV[myLine], clerkLineLock);
                clerkStates[myLine] = BUSY; /*redundant setting*/
            }else{
                Acquire(breakLock[myLine]);
                writeWithSize(my_strcat(currentThreadGetName(currentThread), " is going on break\n"));
                clerkStates[myLine] = ONBREAK;
                Release(clerkLineLock);
                Wait(breakCV[myLine], breakLock[myLine]);
                clerkStates[myLine] = AVAILABLE;
                Release(breakLock[myLine]);
            }
        }

    } while(clerkStates[myLine] != BUSY);

    Acquire(clerkLock[myLine]);
    Release(clerkLineLock);
    /*wait for customer*/
    if(testFlag){
      Signal(clerkSenatorCV[myLine], clerkSenatorCVLock[myLine]);
      Release(clerkSenatorCVLock[myLine]);
    }
    Wait(clerkCV[myLine], clerkLock[myLine]);
    /*Do my job -> customer waiting*/
    return customerData[myLine];
}

/* CL: Parameter: int myLine (line number of clerk)
    Summary: completes the final signal wait communication after doing all the logic in choosing customer
    Return value: void */

void clerkSignalsNextCustomer(int myLine) {
    Signal(clerkCV[myLine], clerkLock[myLine]);
    /* If there is a senator, here is where the clerk waits, after senator is done with them */
    Wait(clerkCV[myLine], clerkLock[myLine]);
    Release(clerkLock[myLine]);
}

/* CL: Parameter: int myLine (line number of clerk)
    Summary: logics for application clerk
    Return value: void */

void hasSignaledString(char* threadName, char* personName, int custNumber) {
    writeWithSize(
        my_strcat(
            concatStringWithNumber(
                my_strcat(
                    my_strcat(
                        my_strcat(
                            my_strcat(threadName, " has signalled a ")
                        , personName)
                    , " to come to their counter. (")
                , personName)
            , custNumber)
        , ")\n"));
}

void givenSSNString(char* personName, int custNumber, char* threadName) {
    writeWithSize(
        my_strcat(
            my_strcat(
                my_strcat(
                    concatStringWithNumber(
                        my_strcat(
                            concatStringWithNumber(personName, custNumber)
                        , " has given SSN ")
                    , custNumber)
                , " to ")
            , threadName)
        , "\n"));
}

void recievedSSNString(char* threadName, int custNumber, char* personName) {
    writeWithSize(
        my_strcat(
            concatStringWithNumber(
                my_strcat(
                    my_strcat(
                        concatStringWithNumber(
                            my_strcat(threadName, " has received SSN ")
                        , custNumber)
                    , " from ")
                , personName)
            , custNumber)
        , "\n"));
}

void ApplicationClerk() {
    int myLine = globalClerkCount;
    int custNumber = chooseCustomerFromLine(myLine);
    int i, numYields;
    char personName[50];
    int currentThread = globalThreadCount;
    my_strcpy(threadNames[currentThread], concatStringWithNumber("ApplicationClerk_", currentThread));
    clerkIdMap[myLine] = currentThread;
    ++globalThreadCount;
    ++globalClerkCount;

    while(true) {
        my_strcpy(personName, "Customer_");
        if(custNumber >= 50) {
            my_strcpy(personName,"Senator_\0");
        }
        clerkStates[myLine] = BUSY;
        hasSignaledString(currentThreadGetName(currentThread), personName, custNumber);
        Yield();
        givenSSNString(personName, custNumber, currentThreadGetName(currentThread));
        Yield();
        recievedSSNString(currentThreadGetName(currentThread), custNumber, personName);

/* CL: random time for applicationclerk to process data */
        numYields = Rand(80, 20);
        for(i = 0; i < numYields; ++i) {
            Yield();
        }

        customerAttributes[custNumber].applicationIsFiled = true;
        writeWithSize(my_strcat(concatStringWithNumber(my_strcat(my_strcat(currentThreadGetName(currentThread), " has recorded a completed application for "), personName), custNumber), "\n"));

        clerkSignalsNextCustomer(myLine);
    }
}

/* CL: Parameter: int myLine (line number of clerk)
    Summary: logics for picture clerk
    Return value: void */

void PictureClerk() {
    int myLine = globalClerkCount;
    int custNumber = chooseCustomerFromLine(myLine);
    int i = 0, numYields, probability;
    char personName[50];
    int currentThread = globalThreadCount;
    my_strcpy(threadNames[currentThread], concatStringWithNumber("PictureClerk_", currentThread));
    clerkIdMap[myLine] = currentThread;
    ++globalThreadCount;
    ++globalClerkCount;

    while(true) {
        my_strcpy(personName, "Customer_\0");
        if(custNumber >= 50) {
                my_strcpy(personName, "Senator_\0");
        }
        clerkStates[myLine] = BUSY;
        hasSignaledString(currentThreadGetName(currentThread), personName, custNumber);
        Yield();
        givenSSNString(personName, custNumber, currentThreadGetName(currentThread));
        Yield();
        recievedSSNString(currentThreadGetName(currentThread), custNumber, personName);

        numYields = Rand(80, 20);

        while(!customerAttributes[custNumber].likesPicture) {
            writeWithSize(my_strcat(concatStringWithNumber(my_strcat(my_strcat(currentThreadGetName(currentThread), " has taken a picture of "), personName), custNumber), "\n"));

            probability = Rand(100, 0);
            if(probability >= 25) {
                customerAttributes[custNumber].likesPicture = true;
                writeWithSize(my_strcat(my_strcat(my_strcat(concatStringWithNumber("Customer_", custNumber), " does like their picture from "), currentThreadGetName(currentThread)), "\n"));
                Yield();
                writeWithSize(my_strcat(concatStringWithNumber(my_strcat(currentThreadGetName(currentThread), " has been told that Customer_"), custNumber), " does like their picture\n"));
                /* CL: random time for pictureclerk to process data */

                for(i = 0; i < numYields; ++i) {
                    Yield();
                }
            }else{
                writeWithSize(my_strcat(my_strcat(my_strcat(concatStringWithNumber(personName, custNumber), " does not like their picture from "), currentThreadGetName(currentThread)), "\n"));
                writeWithSize(my_strcat(concatStringWithNumber(my_strcat(my_strcat(currentThreadGetName(currentThread), " has been told that "), personName), custNumber), " does not like their picture\n"));
            }
        }
        clerkSignalsNextCustomer(myLine);
    }
}

/* CL: Parameter: int myLine (line number of clerk)
    Summary: logics for passport clerk
    Return value: void */

void PassportClerk() {
    int myLine = globalClerkCount;
    int custNumber = chooseCustomerFromLine(myLine);
    int numYields, clerkMessedUp, i;
    char personName[50];
    int currentThread = globalThreadCount;
    my_strcpy(threadNames[currentThread], concatStringWithNumber("PassportClerk_", currentThread));
    clerkIdMap[myLine] = currentThread;
    ++globalThreadCount;
    ++globalClerkCount;

    while(true) {
        my_strcpy(personName, "Customer_");
        if(custNumber >= 50) {
            my_strcpy(personName, "Senator_");
        }
        hasSignaledString(currentThreadGetName(currentThread), personName, custNumber);
        Yield();
        givenSSNString(personName, custNumber, currentThreadGetName(currentThread));
        Yield();
        recievedSSNString(currentThreadGetName(currentThread), custNumber, personName);
        if(customerAttributes[custNumber].likesPicture && customerAttributes[custNumber].applicationIsFiled) {
            /* CL: only determine that customer has app and pic completed by the boolean */
            writeWithSize(my_strcat(concatStringWithNumber(my_strcat(my_strcat(currentThreadGetName(currentThread), " has determined that "), personName), custNumber), " has both their application and picture completed\n"));
            clerkStates[myLine] = BUSY;

            numYields = Rand(80, 20);

                clerkMessedUp = Rand(100, 0);
                        if(custNumber > 49){
              clerkMessedUp = 100;
            }
            if(clerkMessedUp <= 5) { /* Send to back of line */
                writeWithSize(my_strcat(concatStringWithNumber(my_strcat(my_strcat(currentThreadGetName(currentThread), ": Messed up for "), personName), custNumber), ". Sending customer to back of line.\n"));
                customerAttributes[custNumber].clerkMessedUp = true; /*TODO: customer uses this to know which back line to go to*/
            } else {
                writeWithSize(my_strcat(concatStringWithNumber(my_strcat(my_strcat(currentThreadGetName(currentThread), " has recorded "), personName), custNumber), " passport documentation\n"));
                for(i = 0; i < numYields; ++i) {
                    Yield();
                }
                customerAttributes[custNumber].clerkMessedUp = false;
                customerAttributes[custNumber].hasCertification = true;
            }
        } else {
            writeWithSize(my_strcat(concatStringWithNumber(my_strcat(my_strcat(currentThreadGetName(currentThread), " has determined that "), personName), custNumber), " does not have both their application and picture completed\n"));
        }
        clerkSignalsNextCustomer(myLine);
    }
}

/*CL: Parameter: int myLine (line number of clerk)
    Summary: logics for cashier
    Return value: void*/

void Cashier() {
    int myLine = globalClerkCount;
    int custNumber = chooseCustomerFromLine(myLine);
    int numYields, clerkMessedUp, i;
    char personName[50];
    int currentThread = globalThreadCount;
    my_strcpy(threadNames[currentThread], concatStringWithNumber("Cashier_", currentThread));
    clerkIdMap[myLine] = currentThread;
    ++globalThreadCount;
    ++globalClerkCount;

    while(true) {
        my_strcpy(personName, "Customer_");
        if(custNumber >= 50) {
            my_strcpy(personName, "Senator_");
        }
        hasSignaledString(currentThreadGetName(currentThread), personName, custNumber);
        Yield();
        givenSSNString(personName, custNumber, currentThreadGetName(currentThread));
        Yield();
        recievedSSNString(currentThreadGetName(currentThread), custNumber, personName);
        if(customerAttributes[custNumber].hasCertification) {
            writeWithSize(my_strcat(concatStringWithNumber(my_strcat(my_strcat(currentThreadGetName(currentThread), " has verified that "), personName), custNumber), "has been certified by a PassportClerk\n"));
            customerAttributes[custNumber].money -= 100;/* CL: cashier takes money from customer */
            writeWithSize(my_strcat(concatStringWithNumber(my_strcat(my_strcat(currentThreadGetName(currentThread), " has received the $100 from "), personName), custNumber), "after certification\n"));
            writeWithSize(my_strcat(my_strcat(my_strcat(concatStringWithNumber(personName, custNumber), " has given "), currentThreadGetName(currentThread)), " $100\n"));

            clerkMoney[myLine] += 100;
            clerkStates[myLine] = BUSY;
            numYields = Rand(80, 20);
            /* CL: yields after processing money*/
            for(i = 0; i < numYields; ++i) {
                Yield();
            }
            clerkMessedUp = Rand(100, 0);
                           if(custNumber > 49){
              clerkMessedUp = 100;
            }
            if(clerkMessedUp <= 5) { /* Send to back of line*/
                writeWithSize(my_strcat(concatStringWithNumber(my_strcat(my_strcat(currentThreadGetName(currentThread), ": Messed up for "), personName), custNumber), ". Sending customer to back of line.\n"));
                customerAttributes[custNumber].clerkMessedUp = true; /* TODO: customer uses this to know which back line to go to*/
            } else {
                writeWithSize(my_strcat(concatStringWithNumber(my_strcat(my_strcat(currentThreadGetName(currentThread), " has provided "), personName), custNumber), "their completed passport\n"));
                Yield();
                writeWithSize(my_strcat(concatStringWithNumber(my_strcat(my_strcat(currentThreadGetName(currentThread), " has recorded that "), personName), custNumber), " has been given their completed passport\n"));
                customerAttributes[custNumber].clerkMessedUp = false;
                customerAttributes[custNumber].isDone = true;
            }
        }
        clerkSignalsNextCustomer(myLine);
    }
}

/*CL: Parameter: int custNumber
    Summary: logics for customer, includes logic to choose lines to go to and to bribe or not
    Return value: void*/

void Customer() {
    int yieldTime, i;
    int bribe = false; /*HUNG: flag to know whether the customer has paid the bribe, can't be arsed to think of a more elegant way of doing this*/
    int myLine = -1;
    int lineSize = 1000;
    int pickedApplication;
    int pickedPicture;
    int totalLineCount;
    int custNumber = globalCustCount;
    struct CustomerAttribute myCustAtt = initCustAttr(custNumber); /*Hung: Creating a CustomerAttribute for each new customer*/
    int currentThread = globalThreadCount;
    my_strcpy(threadNames[currentThread], concatStringWithNumber("Customer_", custNumber));
    customerIdMap[custNumber] = currentThread;
    ++globalThreadCount;
    ++globalCustCount;

    customerAttributes[custNumber] = myCustAtt;
    while(!customerAttributes[custNumber].isDone) {
        if(senatorLineCount > 0){
            Acquire(outsideLock);
            Wait(outsideLineCV, outsideLock);
            Release(outsideLock);
        }
        Acquire(clerkLineLock); /* CL: acquire lock so that only this customer can access and get into the lines*/

        if(!customerAttributes[custNumber].applicationIsFiled && !customerAttributes[custNumber].likesPicture) { /* check conditions if application and picture are done*/
            pickedApplication = Rand(2, 0);
            pickedPicture = !pickedApplication;
        } else {
            pickedApplication = true;
            pickedPicture = true;
        }
        for(i = 0; i < clerkCount; i++) {
            totalLineCount = clerkLineCount[i] + clerkBribeLineCount[i];

            /* CL: if else pairs for customer to choose clerk based on their attributes*/
            if(pickedApplication &&
                !customerAttributes[custNumber].applicationIsFiled &&
                !customerAttributes[custNumber].hasCertification &&
                !customerAttributes[custNumber].isDone &&
                clerkTypes[i] == "ApplicationClerk") {

                if(totalLineCount < lineSize ) {
                    myLine = i;
                    lineSize = totalLineCount;
                }
            } else if(pickedPicture &&
                      !customerAttributes[custNumber].likesPicture &&
                      !customerAttributes[custNumber].hasCertification &&
                      !customerAttributes[custNumber].isDone &&
                      clerkTypes[i] == "PictureClerk") {
                if(totalLineCount < lineSize) {
                    myLine = i;
                    lineSize = totalLineCount;
                }
            } else if(customerAttributes[custNumber].applicationIsFiled &&
                      customerAttributes[custNumber].likesPicture &&
                      !customerAttributes[custNumber].hasCertification &&
                      !customerAttributes[custNumber].isDone &&
                      clerkTypes[i] == "PassportClerk") {
                if(totalLineCount < lineSize) {
                    myLine = i;
                    lineSize = totalLineCount;
                }
            } else if(customerAttributes[custNumber].applicationIsFiled &&
                      customerAttributes[custNumber].likesPicture &&
                      customerAttributes[custNumber].hasCertification &&
                      !customerAttributes[custNumber].isDone &&
                      clerkTypes[i] == "Cashier") {
                if(totalLineCount < lineSize) {
                    myLine = i;
                    lineSize = totalLineCount;
                }
            }
        }

        if(clerkStates[myLine] != AVAILABLE ) { /*clerkStates[myLine] == BUSY*/
            /*I must wait in line*/
            if(customerAttributes[custNumber].money > 100){
                writeWithSize(my_strcat(concatStringWithNumber(my_strcat(my_strcat(my_strcat(currentThreadGetName(currentThread), " has gotten in bribe line for "), clerkTypes[myLine]), "_"), myLine), "\n"));
                /* CL: takes bribe money*/
                customerAttributes[custNumber].money -= 500;
                clerkMoney[myLine] += 500;
                clerkBribeLineCount[myLine]++;
                bribe = true;
                Wait(clerkBribeLineCV[myLine], clerkLineLock);
            } else {
                writeWithSize(my_strcat(concatStringWithNumber(my_strcat(my_strcat(my_strcat(currentThreadGetName(currentThread), " has gotten in regular line for "), clerkTypes[myLine]), "_"), myLine), "\n"));
                clerkLineCount[myLine]++;
                Wait(clerkLineCV[myLine], clerkLineLock);
            }

            totalLineCount = 0;
            for(i = 0; i < clerkCount; ++i) {
                totalLineCount = totalLineCount + clerkBribeLineCount[i] + clerkLineCount[i];
            }

            if(bribe) {
                clerkBribeLineCount[myLine]--;
            } else {
                clerkLineCount[myLine]--;
            }

        } else {
            clerkStates[myLine] = BUSY;
        }
        Release(clerkLineLock);

        Acquire(clerkLock[myLine]);
        /*Give my data to my clerk*/
        customerData[myLine] = custNumber;

        Signal(clerkCV[myLine], clerkLock[myLine]);
        /*wait for clerk to do their job*/
        Wait(clerkCV[myLine], clerkLock[myLine]);
       /*Read my data*/
        Signal(clerkCV[myLine], clerkLock[myLine]);
        Release(clerkLock[myLine]);

        if(customerAttributes[custNumber].clerkMessedUp) {
            writeWithSize("Clerk messed up.  Customer is going to the back of the line.\n");
            yieldTime = Rand(900, 100);
            for(i = 0; i < yieldTime; ++i) {
                Yield();
            }
            customerAttributes[custNumber].clerkMessedUp = false;
        }
    }
    /* CL: CUSTOMER IS DONE! YAY!*/
    writeWithSize(my_strcat(currentThreadGetName(currentThread), " is leaving the Passport Office.\n"));
}

/*CL: Parameter: int custNumber (because we treat senators as customers)
    Summary: logics for senators, includes logic to choose lines to go to, very similar to customer but a lot more conditions and locks
    Return value: void*/

void Senator(){
    int custNumber = globalCustCount;
    struct CustomerAttribute myCustAtt = initCustAttr(custNumber); /*Hung: custNumber == 50 to 59*/
    int i, myLine;
    int currentThread = globalThreadCount;
    my_strcpy(threadNames[currentThread], concatStringWithNumber("Senator_", custNumber));
    customerIdMap[custNumber] = currentThread;
    ++globalThreadCount;
    ++globalCustCount;

    customerAttributes[custNumber] = myCustAtt;

    Acquire(senatorLock);
    if(senatorLineCount > 0){
        senatorLineCount++;
        Wait(senatorLineCV, senatorLock);
        senatorDone = true;
        for(i = 0; i < clerkCount; i++){
            Acquire(clerkLock[i]);
            Acquire(clerkSenatorCVLock[i]);
        }
        for(i = 0; i < clerkCount; i++){
            Signal(clerkCV[i], clerkLock[i]);
            Release(clerkLock[i]);
        }
        for(i = 0; i < clerkCount; i++){

            writeWithSize(my_strcat(concatStringWithNumber("Waiting for clerk ", i), "\n"));
            Signal(clerkSenatorCV[i], clerkSenatorCVLock[i]);

            Wait(clerkSenatorCV[i], clerkSenatorCVLock[i]);
            writeWithSize(my_strcat(concatStringWithNumber("Getting confirmation from clerk ", i), "\n"));
        }
        senatorDone=false;
    }else{
        for(i = 0; i < clerkCount; i++){
            Acquire(clerkSenatorCVLock[i]);
        }

        senatorLineCount++;
        Release(senatorLock);
        senatorDone = false;

        for(i = 0; i < clerkCount; i++){
            writeWithSize(my_strcat(concatStringWithNumber("Waiting for clerk ", i), "\n"));
            Wait(clerkSenatorCV[i], clerkSenatorCVLock[i]);
            writeWithSize(my_strcat(concatStringWithNumber("Getting confirmation from clerk ", i), "\n"));
        }
    }

    myLine = 0;
    while(!customerAttributes[custNumber].isDone) {
        for(i = 0; i < clerkCount; i++) {
            if(!customerAttributes[custNumber].applicationIsFiled &&
                /*customerAttributes[custNumber].likesPicture &&*/
                !customerAttributes[custNumber].hasCertification &&
                !customerAttributes[custNumber].isDone &&
                clerkTypes[i] == "ApplicationClerk") {
                writeWithSize(my_strcat(my_strcat("    ", currentThreadGetName(currentThread)), "::: ApplicationClerk chosen\n"));
                myLine = i;
            } else if(/*customerAttributes[custNumber].applicationIsFiled &&*/
                      !customerAttributes[custNumber].likesPicture &&
                      !customerAttributes[custNumber].hasCertification &&
                      !customerAttributes[custNumber].isDone &&
                      clerkTypes[i] == "PictureClerk") {
                writeWithSize(my_strcat(my_strcat("    ", currentThreadGetName(currentThread)), "::: PictureClerk chosen\n"));
                myLine = i;
            } else if(customerAttributes[custNumber].applicationIsFiled &&
                      customerAttributes[custNumber].likesPicture &&
                      !customerAttributes[custNumber].hasCertification &&
                      !customerAttributes[custNumber].isDone &&
                      clerkTypes[i] == "PassportClerk") {
                writeWithSize(my_strcat(my_strcat("    ", currentThreadGetName(currentThread)), "::: PassportClerk chosen\n"));
                myLine = i;
            } else if(customerAttributes[custNumber].applicationIsFiled &&
                      customerAttributes[custNumber].likesPicture &&
                      customerAttributes[custNumber].hasCertification &&
                      !customerAttributes[custNumber].isDone &&
                      clerkTypes[i] == "Cashier") {
                writeWithSize(my_strcat(my_strcat("    ", currentThreadGetName(currentThread)), "::: Cashier chosen\n"));
                myLine = i;
            }
        }
        Signal(clerkSenatorCV[myLine], clerkSenatorCVLock[myLine]);
        writeWithSize(my_strcat(concatStringWithNumber(my_strcat(my_strcat(my_strcat(currentThreadGetName(currentThread), "has gotten in regular line for "), clerkTypes[myLine]), "_"), myLine), "\n"));
        Wait(clerkSenatorCV[myLine], clerkSenatorCVLock[myLine]);

        Release(clerkSenatorCVLock[myLine]);
        Acquire(clerkLock[myLine]);
        /*Give my data to my clerk*/
        customerData[myLine] = custNumber;
        Signal(clerkCV[myLine], clerkLock[myLine]);

        /*wait for clerk to do their job*/
        Wait(clerkCV[myLine], clerkLock[myLine]);
        /*Read my data*/
    }

    Acquire(senatorLock);
    senatorLineCount--;
    if(senatorLineCount == 0){
        for(i = 0 ; i < clerkCount ; i++){
            /*Free up clerks that worked with me*/
            Signal(clerkCV[i], clerkLock[i]);
            /*Frees all locks I hold*/
            Release(clerkLock[i]);
            /*Free up clerks that I didn't work with*/
            Signal(clerkSenatorCV[i], clerkSenatorCVLock[i]);
            /*Frees up all clerkSenatorCVLocks*/
            Release(clerkSenatorCVLock[i]);
        }
        Acquire(outsideLock);
        Broadcast(outsideLineCV, outsideLock);
        Release(outsideLock);
        Release(senatorLock);
    }else{
        for(i = 0 ; i < clerkCount ; i++){
            /*Free up clerkLocks for other senator*/
            Release(clerkLock[i]);
            /*Frees up clerkSenatorCVLocks for other senator*/
            Release(clerkSenatorCVLock[i]);
        }
        Signal(senatorLineCV, senatorLock);
        Release(senatorLock);
    }
    writeWithSize(my_strcat(currentThreadGetName(currentThread), " is leaving the Passport Office.\n"));
}

void wakeUpClerks() {
    int i;
    for(i = 0; i < clerkCount; ++i) {
        if(clerkStates[i] == ONBREAK) {
            writeWithSize(concatStringWithNumber(my_strcat(my_strcat("Manager has woken up a ", clerkTypes[i]), "_"), i));
            Acquire(breakLock[i]);
            Signal(breakCV[i], breakLock[i]);
            Release(breakLock[i]);
            writeWithSize(my_strcat(concatStringWithNumber(my_strcat(clerkTypes[i], "_"), i), " is coming off break\n"));
        }
    }
}

/* CL: Parameter: -
    Summary: print all the money as manager checks the money earned by each clerk
    Return value: void */

void printMoney() {
    int totalMoney = 0;
    int applicationMoney = 0;
    int pictureMoney = 0;
    int passportMoney = 0;
    int cashierMoney = 0;
    int i;
    for(i = 0; i < clerkCount; ++i) {
        if (i < clerkArray[0]){ /*ApplicationClerk index*/
            applicationMoney += clerkMoney[i];
        } else if (i < clerkArray[0] + clerkArray[1]){ /*PictureClerk index*/
            pictureMoney += clerkMoney[i];
        } else if (i < clerkArray[0] + clerkArray[1] + clerkArray[2]){ /*PassportClerk index*/
            passportMoney += clerkMoney[i];
        } else if (i < clerkArray[0] + clerkArray[1] + clerkArray[2] + clerkArray[3]){ /*Cashier index*/
            cashierMoney += clerkMoney[i];
        }
        totalMoney += clerkMoney[i];
    }


    writeWithSize(my_strcat(concatStringWithNumber("Manager has counted a total of ", applicationMoney), " for ApplicationClerks\n"));
    writeWithSize(my_strcat(concatStringWithNumber("Manager has counted a total of ",pictureMoney), " for PictureClerks\n"));
    writeWithSize(my_strcat(concatStringWithNumber("Manager has counted a total of ", passportMoney), " for PassportClerks\n"));
    writeWithSize(my_strcat(concatStringWithNumber("Manager has counted a total of ", cashierMoney), " for Cashiers\n"));
    writeWithSize(my_strcat(concatStringWithNumber("Manager has counted a total of ", totalMoney), " for the passport office\n\n"));
}

/*CL: Parameter: -
    Summary: manager code, interrupts are disabled
    Return value: void*/

void Manager() {
    int totalLineCount, i, waitTime;
    int currentThread = globalThreadCount;
    my_strcpy(threadNames[currentThread], concatStringWithNumber("Manager_", currentThread));
    ++globalThreadCount;

    do {
        /* IntStatus oldLevel = interrupt->SetLevel(IntOff); disable interrupts*/
        totalLineCount = 0;
        for(i = 0; i < clerkCount; ++i) {
            totalLineCount += clerkLineCount[i] + clerkBribeLineCount[i];
            if(totalLineCount > 2 || senatorLineCount > 0 ) {
                wakeUpClerks();
                break;
            }
        }
        printMoney();
        /* (void) interrupt->SetLevel(oldLevel); /*restore interrupts*/

        waitTime = 100000;
        for(i = 0; i < waitTime; ++i) {
            Yield();
        }
    } while(!customersAreAllDone());
}

/*CL: Parameter: -
    Summary: check if customers are done, i.e. are all attributes set to true
    Return value: void*/

int customersAreAllDone() {
    int boolCount = 0, i;
    for(i = 0; i < customerCount; ++i) {

        boolCount += customerAttributes[i].isDone;
    }

    for(i = 0; i < senatorCount; ++i) {
        i += 50;
        boolCount += customerAttributes[i].isDone;
        i -= 50;
    }

    prevTotalBoolCount = currentTotalBoolCount;
    currentTotalBoolCount = 0;
    for(i = 0; i < customerCount; ++i) {
        currentTotalBoolCount += customerAttributes[i].applicationIsFiled + customerAttributes[i].likesPicture + customerAttributes[i].hasCertification + customerAttributes[i].isDone;
    }
    if(prevTotalBoolCount == currentTotalBoolCount) {
        wakeUpClerks();
    }

    if(boolCount == customerCount + senatorCount) {
        return true;
    }
    return false;
}
