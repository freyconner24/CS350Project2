

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
char threadNames[250][80];
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
typedef enum X {AVAILABLE, BUSY, ONBREAK} ClerkState; /* CL: enum for clerk's conditions*/
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
char clerkTypesStatic[CLERK_TYPES][30] = { "ApplicationClerks : ", "PictureClerks     : ", "PassportClerks    : ", "Cashiers          : " };
char clerkTypes[CLERK_NUMBER][30];
int clerkTypesLengths[CLERK_NUMBER];
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

char* my_strcpy(char *s1, const char *s2, int len) {
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
    return threadNames[currentThread];;
}

/* CL: parameter: an int array that contains numbers of each clerk type
     Summary: gets input from user or test, initialize and print out clerk numbers
     return value: void */

void clerkFactory(int countOfEachClerkType[]) {
    int tempClerkCount = 0, i = 0;
    for(i = 0; i < CLERK_TYPES; ++i) {
        if(testChosen == 0) { /* gets input from user if running full program, does not get input if test */
            do {
                PrintString(clerkTypesStatic[i], 20);
                tempClerkCount = 4;  /* Technically would read user input */
                if(tempClerkCount <= 0 || tempClerkCount > 5) {
                    PrintString("    The number of clerks must be between 1 and 5 inclusive!\n", 60);
                }
            } while(tempClerkCount <= 0 || tempClerkCount > 5);
            clerkCount += tempClerkCount;
            clerkArray[i] = tempClerkCount;
        } else {
            clerkArray[i] = countOfEachClerkType[i];
        }
    }
    /* CL: print statements */
    PrintString("Number of ApplicationClerks = ", 30); PrintNum(clerkArray[0]); PrintNl();
    PrintString("Number of PictureClerks = ", 26); PrintNum(clerkArray[1]); PrintNl();
    PrintString("Number of PassportClerks = ", 27); PrintNum(clerkArray[2]); PrintNl();
    PrintString("Number of CashiersClerks = ", 27); PrintNum(clerkArray[3]); PrintNl();
    PrintString("Number of Senators = ", 20); PrintNum(senatorCount); PrintNl();
}

void createClerkThread(int type, int* clerkNumber) {
    char clerkType[50], clerkName[50];
    int i, temp, clerkTypeLength;
    for(i = 0; i < clerkArray[type]; ++i) {
        if(type == 0) {
            clerkTypeLength = 16;
            my_strcpy(clerkType, "ApplicationClerk", clerkTypeLength);
            Fork((VoidFunctionPtr)ApplicationClerk, *clerkNumber);
        } else if(type == 1) {
            clerkTypeLength = 12;
            my_strcpy(clerkType, "PictureClerk", clerkTypeLength);
            Fork((VoidFunctionPtr)PictureClerk, *clerkNumber);
        } else if(type == 2) {
            clerkTypeLength = 13;
            my_strcpy(clerkType, "PassportClerk", clerkTypeLength);
            Fork((VoidFunctionPtr)PassportClerk, *clerkNumber);
        } else if(type == 3) {
            clerkTypeLength = 8;
            my_strcpy(clerkType, "Cashier", clerkTypeLength);
            Fork((VoidFunctionPtr)Cashier, *clerkNumber);
        } else {
            return;
        }

        my_strcpy(clerkTypes[*clerkNumber], clerkType, clerkTypeLength);
        clerkTypesLengths[*clerkNumber] = clerkTypeLength;
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
    char name[20];
    int cpyLen;
    for(i = 0; i < clerkCount; ++i) {
        if(i / 10 == 0) {
            cpyLen = 1;
        } else {
            cpyLen = 2;
        }
        
        clerkLock[i] = CreateLock("ClerkLock", 9 + cpyLen, i);

        clerkCV[i] = CreateCondition("ClerkCV", 7 + cpyLen, i);

        clerkLineCV[i] = CreateCondition("ClerkLineCV", 11 + cpyLen, i);

        clerkBribeLineCV[i] = CreateCondition("ClerkBribeLineCV", 16 + cpyLen, i);

        breakLock[i] = CreateLock("BreakLock", 9 + cpyLen, i);

        breakCV[i] = CreateCondition("BreakCV", 7 + cpyLen, i);

        clerkSenatorCV[i] = CreateCondition("ClerkSenatorCV", 14 + cpyLen, i);

        clerkSenatorCVLock[i] = CreateLock("ClerkSenatorCV", 14 + cpyLen, i);
    }
}

/*CL: Parameter: Thread*
    Summary: create and fire off customer threads with designated names
    Return value: void*/
void createCustomerThreads() {
    int i;
    for(i = 0; i < customerCount; i++){
        Fork((VoidFunctionPtr)Customer, i);
    }
}

/*CL: Parameter: Thread*
    Summary: create and fire off senator threads with designated names
    Return value: void*/
void createSenatorThreads(){
    int i;
    for(i = 0; i < senatorCount; i++){
        Fork((VoidFunctionPtr)Senator, i + 50);
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
    Fork((VoidFunctionPtr)Manager, 0);
}

void Part2() {
    char *name;
    int countOfEachClerkType[CLERK_TYPES] = {0,0,0,0};

    PrintString("Starting Part 2\n", 16);
    PrintString("Test to run (put 0 for full program): ", 38);
    testChosen = 1; /* technically cin >> */

    if(testChosen == 1) {
        PrintString("Starting Test 1\n", 16); /*Customers always take the shortest line, but no 2 customers ever choose the same shortest line at the same time*/
        customerCount = 10;
        clerkCount = 6;
        senatorCount = 0;
        countOfEachClerkType[0] = 2; countOfEachClerkType[1] = 2; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 2) {
        PrintString("Starting Test 2\n", 16); /*Managers only read one from one Clerk's total money received, at a time*/
        customerCount = 5;
        clerkCount = 4;
        senatorCount = 0;
        countOfEachClerkType[0] = 1; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 3) {
        PrintString("Starting Test 3\n", 16); /*Customers do not leave until they are given their passport by the Cashier.
                                     The Cashier does not start on another customer until they know that the last Customer has left their area*/
        customerCount = 5;
        clerkCount = 4;
        senatorCount = 0;
        countOfEachClerkType[0] = 1; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 4) {
        PrintString("Starting Test 4\n", 16); /*Clerks go on break when they have no one waiting in their line*/
        customerCount = 5;
        clerkCount = 4;
        senatorCount = 0;
        countOfEachClerkType[0] = 1; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 5) {
        PrintString("Starting Test 5\n", 16); /*Managers get Clerks off their break when lines get too long*/
        customerCount = 7;
        clerkCount = 4;
        senatorCount = 0;
        countOfEachClerkType[0] = 1; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 6) {
        PrintString("Starting Test 6\n", 16); /*Total sales never suffers from a race condition*/
        customerCount = 25;
        clerkCount = 4;
        senatorCount = 0;
        countOfEachClerkType[0] = 1; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 7) {
        PrintString("Starting Test 7\n", 16); /*Total sales never suffers from a race condition*/
        customerCount = 7;
        clerkCount = 4;
        senatorCount = 1;
        countOfEachClerkType[0] = 1; countOfEachClerkType[1] = 1; countOfEachClerkType[2] = 1; countOfEachClerkType[3] = 1;

        createTestVariables(countOfEachClerkType);
    } else if(testChosen == 0) {
        do {
            PrintString("Number of customers: ", 21);
            customerCount = 20; /* technically cin >> */
            if(customerCount <= 0 || customerCount > 50) {
                PrintString("    The number of customers must be between 1 and 50 inclusive!\n", 64);
            }
        } while(customerCount <= 0 || customerCount > 50);

        do {
            PrintString("Number of Senators: ", 20);
            senatorCount = 1; /* technically cin >> */
            if(senatorCount < 0 || senatorCount > 10) {
                PrintString("    The number of senators must be between 1 and 10 inclusive!\n", 63);
            }
        } while(senatorCount < 0 || senatorCount > 10);

        createTestVariables(countOfEachClerkType);
    }
}

/* CL: Parameter: int myLine (line number of current clerk)
    Summary: chooses customer from the line, or decides if clerks go on break
    Return value: int (the customer number) */

int chooseCustomerFromLine(int myLine, char* clerkName, int clerkNameLength) {
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
                PrintString(clerkName, clerkNameLength); PrintNum(myLine); PrintString(" is servicing a customer from regular line\n", 43);
                Signal(clerkLineCV[myLine], clerkLineLock);
                clerkStates[myLine] = BUSY; /*redundant setting*/
            }else{
                Acquire(breakLock[myLine]);
                PrintString(clerkName, clerkNameLength); PrintNum(myLine); PrintString(" is going on break\n", 19);
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

void hasSignaledString(char* threadName, int threadNameLength, int clerkNum, char* personName, int personNameLength, int custNumber) {
    PrintString(threadName, threadNameLength); PrintNum(clerkNum); PrintString(" has signalled a ", 17); 
    PrintString(personName, personNameLength); PrintString(" to come to their counter. (", 28);
    PrintString(personName, personNameLength); PrintNum(custNumber); PrintNl();
}

void givenSSNString(char* personName, int personNameLength, int custNumber, char* threadName, int threadNameLength, int clerkNum) {
    PrintString(personName, personNameLength); PrintNum(custNumber); PrintString(" has given SSN ", 15);
    PrintNum(custNumber); PrintString(" to ", 4); PrintString(threadName, threadNameLength); PrintNum(clerkNum); PrintNl();
}

void recievedSSNString(char* threadName, int threadNameLength, int clerkNum, int custNumber, char* personName, int personNameLength) {
    PrintString(threadName, threadNameLength); PrintNum(clerkNum); PrintString(" has received SSN ", 18); PrintNum(custNumber);
    PrintString(" from ", 6); PrintString(personName, personNameLength); PrintNum(custNumber); PrintNl();
}

void ApplicationClerk() {
    int myLine = GetThreadArgs();
    int i, numYields, personNameLength;
    char personName[50];
    int currentThread = globalThreadCount;
    my_strcpy(threadNames[currentThread], concatStringWithNumber("ApplicationClerk_", myLine), 0); /* TODO: change 0 */
    clerkIdMap[myLine] = currentThread;
    ++globalThreadCount;

    while(true) {
        int custNumber = chooseCustomerFromLine(myLine, "ApplicationClerk_", 17);
        my_strcpy(personName, "Customer_", 9);
        personNameLength = 9;
        if(custNumber >= 50) {
            my_strcpy(personName,"Senator_", 8);
            personNameLength = 8;
        }
        clerkStates[myLine] = BUSY;
        hasSignaledString("ApplicationClerk_", 17, myLine, personName, personNameLength, custNumber);
        Yield();
        givenSSNString(personName, personNameLength, custNumber, "ApplicationClerk_", 17, myLine);
        Yield();
        recievedSSNString("ApplicationClerk_", 17, myLine, custNumber, personName, personNameLength);

/* CL: random time for applicationclerk to process data */
        numYields = Rand(80, 20);
        for(i = 0; i < numYields; ++i) {
            Yield();
        }

        customerAttributes[custNumber].applicationIsFiled = true;
        PrintString("ApplicationClerk_", 17); PrintNum(myLine); 
        PrintString(" has recorded a completed application for ", 42); 
        PrintString(personName, personNameLength); PrintNum(custNumber); PrintNl();

        clerkSignalsNextCustomer(myLine);
    }
    Exit(0);
}

/* CL: Parameter: int myLine (line number of clerk)
    Summary: logics for picture clerk
    Return value: void */

void PictureClerk() {
    int myLine = GetThreadArgs();
    int i = 0, numYields, probability, personNameLength;
    char personName[50];
    int currentThread = globalThreadCount;
    my_strcpy(threadNames[currentThread], concatStringWithNumber("PictureClerk_", currentThread), 0);
    clerkIdMap[myLine] = currentThread;
    ++globalThreadCount;

    while(true) {
        int custNumber = chooseCustomerFromLine(myLine, "PictureClerk_", 13);
        my_strcpy(personName, "Customer_", 9);
        personNameLength = 9;
        if(custNumber >= 50) {
                my_strcpy(personName, "Senator_", 8);
                personNameLength = 8;
        }
        clerkStates[myLine] = BUSY;
        hasSignaledString("PictureClerk_", 13, myLine, personName, personNameLength, custNumber);
        Yield();
        givenSSNString(personName, personNameLength, custNumber, "PictureClerk_", 13, myLine);
        Yield();
        recievedSSNString("PictureClerk_", 13, myLine, custNumber, personName, personNameLength);

        numYields = Rand(80, 20);

        while(!customerAttributes[custNumber].likesPicture) {
            PrintString("PictureClerk_", 13); PrintNum(myLine); PrintString(" has taken a picture of ", 24);
            PrintString(personName, personNameLength); PrintNum(custNumber); PrintNl();

            probability = Rand(100, 0);
            if(probability >= 25) {
                customerAttributes[custNumber].likesPicture = true;
                PrintString("Customer_", 9); PrintNum(custNumber);  PrintString(" does like their picture from ", 30); PrintString("PictureClerk_", 13); PrintNum(myLine); PrintNl();
                Yield();
                PrintString("PictureClerk_", 13); PrintNum(myLine); PrintString(" has been told that Customer_", 29); PrintNum(custNumber); PrintString(" does like their picture\n", 25);
                /* CL: random time for pictureclerk to process data */

                for(i = 0; i < numYields; ++i) {
                    Yield();
                }
            }else{
                PrintString(personName, personNameLength); PrintNum(custNumber); PrintString(" does not like their picture from ", 34); PrintString("PictureClerk_", 13); PrintNum(myLine); PrintNl();
                PrintString("PictureClerk_", 13); PrintNum(myLine); PrintString(" has been told that ", 20); PrintString(personName, personNameLength); PrintNum(custNumber); PrintString(" does not like their picture\n", 29);
            }
        }
        clerkSignalsNextCustomer(myLine);
    }
    Exit(0);
}

/* CL: Parameter: int myLine (line number of clerk)
    Summary: logics for passport clerk
    Return value: void */

void PassportClerk() {
    int myLine = GetThreadArgs();
    int numYields, clerkMessedUp, i, personNameLength;
    char personName[50];
    int currentThread = globalThreadCount;
    my_strcpy(threadNames[currentThread], concatStringWithNumber("PassportClerk_", currentThread), 0);
    clerkIdMap[myLine] = currentThread;
    ++globalThreadCount;

    while(true) {
        int custNumber = chooseCustomerFromLine(myLine, "PassportClerk_", 14);
        my_strcpy(personName, "Customer_", 9);
        if(custNumber >= 50) {
            my_strcpy(personName, "Senator_", 8);
        }
        hasSignaledString("PassportClerk_", 14, myLine, personName, personNameLength, custNumber);
        Yield();
        givenSSNString(personName, personNameLength, custNumber, "PassportClerk_", 14, myLine);
        Yield();
        recievedSSNString("PassportClerk_", 14, myLine, custNumber, personName, personNameLength);
        if(customerAttributes[custNumber].likesPicture && customerAttributes[custNumber].applicationIsFiled) {
            /* CL: only determine that customer has app and pic completed by the boolean */
            PrintString("PassportClerk_", 14); PrintNum(myLine); PrintString(" has determined that ", 21); PrintString(personName, personNameLength); PrintNum(custNumber); PrintString(" has both their application and picture completed\n",50);
            clerkStates[myLine] = BUSY;

            numYields = Rand(80, 20);

                clerkMessedUp = Rand(100, 0);
                        if(custNumber > 49){
              clerkMessedUp = 100;
            }
            if(clerkMessedUp <= 5) { /* Send to back of line */
                PrintString("PassportClerk_", 14); PrintNum(myLine); PrintString(": Messed up for ", 16);
                PrintString(personName, personNameLength); PrintNum(custNumber); PrintString(". Sending customer to back of line.\n", 36);
                customerAttributes[custNumber].clerkMessedUp = true; /*TODO: customer uses this to know which back line to go to*/
            } else {
                PrintString("PassportClerk_", 14); PrintNum(myLine); PrintString(" has recorded ", 14); 
                PrintString(personName, personNameLength); PrintNum(custNumber); PrintString(" passport documentation\n", 24);
                for(i = 0; i < numYields; ++i) {
                    Yield();
                }
                customerAttributes[custNumber].clerkMessedUp = false;
                customerAttributes[custNumber].hasCertification = true;
            }
        } else {
            PrintString("PassportClerk_", 14); PrintNum(myLine); PrintString(" has determined that ", 21);
            PrintString(personName, personNameLength); PrintNum(custNumber); PrintString(" does not have both their application and picture completed\n", 60);
        }
        clerkSignalsNextCustomer(myLine);
    }
    Exit(0);
}

/*CL: Parameter: int myLine (line number of clerk)
    Summary: logics for cashier
    Return value: void*/

void Cashier() {
    int myLine = GetThreadArgs();
    int numYields, clerkMessedUp, i, personNameLength;
    char personName[50];
    int currentThread = globalThreadCount;
    my_strcpy(threadNames[currentThread], concatStringWithNumber("Cashier_", currentThread), 0);
    clerkIdMap[myLine] = currentThread;
    ++globalThreadCount;

    while(true) {
        int custNumber = chooseCustomerFromLine(myLine, "Cashier_", 8);
        my_strcpy(personName, "Customer_", 9);
        personNameLength = 9;
        if(custNumber >= 50) {
            my_strcpy(personName, "Senator_", 8);
            personNameLength = 8;
        }
        hasSignaledString("Cashier_", 8, myLine, personName, personNameLength, custNumber);
        Yield();
        givenSSNString(personName, personNameLength, custNumber, "Cashier_", 8, myLine);
        Yield();
        recievedSSNString("Cashier_", 8, myLine, custNumber, personName, personNameLength);
        if(customerAttributes[custNumber].hasCertification) {
            PrintString("Cashier_", 8); PrintNum(myLine); PrintString(" has verified that ", 19);
            PrintString(personName, personNameLength); PrintNum(custNumber); PrintString("has been certified by a PassportClerk\n", 38);
            customerAttributes[custNumber].money -= 100;/* CL: cashier takes money from customer */
            PrintString("Cashier_", 8); PrintNum(myLine); PrintString(" has received the $100 from ", 28);
            PrintString(personName, personNameLength); PrintNum(custNumber); PrintString("after certification\n", 20);
            PrintString(personName, personNameLength); PrintNum(custNumber); PrintString(" has given ", 11); PrintString("Cashier_", 8); PrintNum(myLine); PrintString(" $100\n", 6);

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
                PrintString("Cashier_", 8); PrintNum(myLine); PrintString(": Messed up for ", 16); 
                PrintString(personName, personNameLength); PrintNum(custNumber);
                PrintString(". Sending customer to back of line.\n", 36);
                customerAttributes[custNumber].clerkMessedUp = true; /* TODO: customer uses this to know which back line to go to*/
            } else {
                PrintString("Cashier_", 8); PrintNum(myLine); PrintString(" has provided ", 11);
                PrintString(personName, personNameLength); PrintNum(custNumber); 
                PrintString("their completed passport\n", 25);
                Yield();
                PrintString("Cashier_", 8); PrintNum(myLine); PrintString(" has recorded that ", 19);
                PrintString(personName, personNameLength); PrintNum(custNumber);
                PrintString(" has been given their completed passport\n", 41);
                customerAttributes[custNumber].clerkMessedUp = false;
                customerAttributes[custNumber].isDone = true;
            }
        }
        clerkSignalsNextCustomer(myLine);
    }
    Exit(0);
}

/*CL: Parameter: int custNumber
    Summary: logics for customer, includes logic to choose lines to go to and to bribe or not
    Return value: void*/

void Customer() {
    int custNumber = GetThreadArgs();
    int yieldTime, i;
    int bribe = false; /*HUNG: flag to know whether the customer has paid the bribe, can't be arsed to think of a more elegant way of doing this*/
    int myLine = -1;
    int lineSize = 1000;
    int pickedApplication;
    int pickedPicture;
    int totalLineCount;
    struct CustomerAttribute myCustAtt = initCustAttr(custNumber); /*Hung: Creating a CustomerAttribute for each new customer*/
    int currentThread = globalThreadCount;
    my_strcpy(threadNames[currentThread], concatStringWithNumber("Customer_", custNumber), 0);
    customerIdMap[custNumber] = currentThread;
    ++globalThreadCount;

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
                PrintString("Customer_", 9); PrintNum(custNumber); PrintString(" has gotten in bribe line for ", 30);
                PrintString(clerkTypes[myLine], clerkTypesLengths[myLine]); PrintString("_", 1); PrintNum(myLine); PrintNl();
                /* CL: takes bribe money*/
                customerAttributes[custNumber].money -= 500;
                clerkMoney[myLine] += 500;
                clerkBribeLineCount[myLine]++;
                bribe = true;
                Wait(clerkBribeLineCV[myLine], clerkLineLock);
            } else {
                PrintString("Customer_", 9); PrintNum(custNumber); PrintString(" has gotten in regular line for ", 19); 
                PrintString(clerkTypes[myLine], clerkTypesLengths[myLine]); PrintString("_", 1); PrintNum(myLine); PrintNl();
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
            PrintString("Clerk messed up.  Customer is going to the back of the line.\n", 61);
            yieldTime = Rand(900, 100);
            for(i = 0; i < yieldTime; ++i) {
                Yield();
            }
            customerAttributes[custNumber].clerkMessedUp = false;
        }
    }
    /* CL: CUSTOMER IS DONE! YAY!*/
    PrintString("Customer_", 9); PrintNum(custNumber); PrintString(" is leaving the Passport Office.\n", 33);
    Exit(0);
}

/*CL: Parameter: int custNumber (because we treat senators as customers)
    Summary: logics for senators, includes logic to choose lines to go to, very similar to customer but a lot more conditions and locks
    Return value: void*/

void Senator(){
    int custNumber = GetThreadArgs();
    struct CustomerAttribute myCustAtt = initCustAttr(custNumber); /*Hung: custNumber == 50 to 59*/
    int i, myLine;
    int currentThread = globalThreadCount;
    my_strcpy(threadNames[currentThread], concatStringWithNumber("Senator_", custNumber), 0);
    customerIdMap[custNumber] = currentThread;
    ++globalThreadCount;

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

            PrintString("Waiting for clerk ", 18); PrintNum(i); PrintNl();
            Signal(clerkSenatorCV[i], clerkSenatorCVLock[i]);

            Wait(clerkSenatorCV[i], clerkSenatorCVLock[i]);
            PrintString("Getting confirmation from clerk ", 32); PrintNum(i); PrintNl();
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
            PrintString("Waiting for clerk ", 18); PrintNum(i); PrintNl();
            Wait(clerkSenatorCV[i], clerkSenatorCVLock[i]);
            PrintString("Getting confirmation from clerk ", 32); PrintNum(i); PrintNl();
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
                PrintString("    ", 4); PrintString("Senator_", 8); PrintNum(custNumber); PrintString("::: ApplicationClerk chosen\n", 28);
                myLine = i;
            } else if(/*customerAttributes[custNumber].applicationIsFiled &&*/
                      !customerAttributes[custNumber].likesPicture &&
                      !customerAttributes[custNumber].hasCertification &&
                      !customerAttributes[custNumber].isDone &&
                      clerkTypes[i] == "PictureClerk") {
                PrintString("    ", 4); PrintString("Senator_", 8); PrintNum(custNumber); PrintString("::: PictureClerk chosen\n", 24);
                myLine = i;
            } else if(customerAttributes[custNumber].applicationIsFiled &&
                      customerAttributes[custNumber].likesPicture &&
                      !customerAttributes[custNumber].hasCertification &&
                      !customerAttributes[custNumber].isDone &&
                      clerkTypes[i] == "PassportClerk") {
                PrintString("    ", 4); PrintString("Senator_", 8); PrintNum(custNumber); PrintString("::: PassportClerk chosen\n", 25);
                myLine = i;
            } else if(customerAttributes[custNumber].applicationIsFiled &&
                      customerAttributes[custNumber].likesPicture &&
                      customerAttributes[custNumber].hasCertification &&
                      !customerAttributes[custNumber].isDone &&
                      clerkTypes[i] == "Cashier") {
                PrintString("    ", 4); PrintString("Senator_", 8); PrintNum(custNumber); PrintString("::: Cashier chosen\n", 19);
                myLine = i;
            }
        }
        Signal(clerkSenatorCV[myLine], clerkSenatorCVLock[myLine]);
        PrintString("Senator_", 8); PrintNum(custNumber); PrintString("has gotten in regular line for ", 31);
        PrintString(clerkTypes[myLine], clerkTypesLengths[myLine]); PrintString("_", 1); PrintNum(myLine); PrintNl();
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
    PrintString("Senator_", 8); PrintNum(custNumber); PrintString(" is leaving the Passport Office.\n", 33);
    Exit(0);
}

void wakeUpClerks() {
    int i;
    for(i = 0; i < clerkCount; ++i) {
        if(clerkStates[i] == ONBREAK) {
            PrintString("Manager has woken up a ", 23); PrintString(clerkTypes[i], clerkTypesLengths[i]); PrintString("_", 1); PrintNum(i);
            Acquire(breakLock[i]);
            Signal(breakCV[i], breakLock[i]);
            Release(breakLock[i]);
            PrintString(clerkTypes[i], clerkTypesLengths[i]); PrintString("_", 1); PrintNum(i); PrintString(" is coming off break\n", 21);
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


    PrintString("Manager has counted a total of ", 31); PrintNum(applicationMoney); PrintString(" for ApplicationClerks\n", 23);
    PrintString("Manager has counted a total of ", 31); PrintNum(pictureMoney); PrintString(" for PictureClerks\n", 19);
    PrintString("Manager has counted a total of ", 31); PrintNum(passportMoney); PrintString(" for PassportClerks\n", 20);
    PrintString("Manager has counted a total of ", 31); PrintNum(cashierMoney); PrintString(" for Cashiers\n", 14);
    PrintString("Manager has counted a total of ", 31); PrintNum(totalMoney); PrintString(" for passport office\n", 21);
}

/*CL: Parameter: -
    Summary: manager code, interrupts are disabled
    Return value: void*/

void Manager() {
    int totalLineCount, i, waitTime;
    int currentThread = globalThreadCount;
    my_strcpy(threadNames[currentThread], "Manager", 7);
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
    Exit(0);
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

int main() {
    OpenFileId fd;
    int bytesread, lockNum, condNum;
    char buf[20];

    clerkLineLock = CreateLock("ClerkLineLock", 14, 0);
    clerkSenatorLineCV = CreateCondition("ClerkSenatorLineCV", 19, 0);
    outsideLineCV = CreateCondition("OutsideLineCV", 14, 0);
    outsideLock = CreateLock("OutsideLock", 12, 0);
    senatorLock = CreateLock("SenatorLock", 12, 0);
    senatorLineCV = CreateCondition("SenatorLineCV", 13, 0);
    Part2();
    /*Write("Testing Locks\n", 14, ConsoleOutput);

    lockNum = CreateLock("nameLock");
    Acquire(lockNum);
    condNum = CreateCondition("someCondition");
    Signal(lockNum, condNum);
    Broadcast(lockNum, condNum);
    Release(lockNum);
    DestroyLock(lockNum);
    Write("Locks complete\n", 15, ConsoleOutput);
    */
/*      Exec('halt');*/

    Exit(0);
}
