
#ifndef PASSPORTOFFICE_H
#define PASSPORTOFFICE_H

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

struct CustomerAttribute initCustAttr(int ssn);


char* reverse_str(char* string);

char* int_to_str(int num);

char* my_strcat(char *dest, const char *src);

char* my_strcpy(char *s1, const char *s2);

char* concatClerkNameWithNumber(char* clerkName, int clerkNumber);

char* concatStringWithNumber(char* str, int num);

void writeWithSize(char* string);

char* currentThreadGetName(int currentThread);

/* CL: parameter: an int array that contains numbers of each clerk type
     Summary: gets input from user or test, initialize and print out clerk numbers
     return value: void */

void clerkFactory(int countOfEachClerkType[]);

void createClerkThread(int type, int* clerkNumber);

void createClerkThreads();

void createClerkLocksAndConditions();

/*CL: Parameter: Thread*
    Summary: create and fire off customer threads with designated names
    Return value: void*/
void createCustomerThreads();

/*CL: Parameter: Thread*
    Summary: create and fire off senator threads with designated names
    Return value: void*/
void createSenatorThreads();

/*CL: Parameter: Thread*, int []
    Summary: group all create thread/ lock/ condition functions together and fire off manager
    Return value: void*/
void createTestVariables(int countOfEachClerkType[]);

void Part2();

/* CL: Parameter: int myLine (line number of current clerk)
    Summary: chooses customer from the line, or decides if clerks go on break
    Return value: int (the customer number) */

int chooseCustomerFromLine(int myLine);

/* CL: Parameter: int myLine (line number of clerk)
    Summary: completes the final signal wait communication after doing all the logic in choosing customer
    Return value: void */

void clerkSignalsNextCustomer(int myLine);

/* CL: Parameter: int myLine (line number of clerk)
    Summary: logics for application clerk
    Return value: void */

void hasSignaledString(char* threadName, char* personName, int custNumber);

void givenSSNString(char* personName, int custNumber, char* threadName);

void recievedSSNString(char* threadName, int custNumber, char* personName);

void ApplicationClerk();

/* CL: Parameter: int myLine (line number of clerk)
    Summary: logics for picture clerk
    Return value: void */

void PictureClerk();

/* CL: Parameter: int myLine (line number of clerk)
    Summary: logics for passport clerk
    Return value: void */

void PassportClerk();

/*CL: Parameter: int myLine (line number of clerk)
    Summary: logics for cashier
    Return value: void*/

void Cashier();

/*CL: Parameter: int custNumber
    Summary: logics for customer, includes logic to choose lines to go to and to bribe or not
    Return value: void*/

void Customer();

/*CL: Parameter: int custNumber (because we treat senators as customers)
    Summary: logics for senators, includes logic to choose lines to go to, very similar to customer but a lot more conditions and locks
    Return value: void*/

void Senator();

void wakeUpClerks();

/* CL: Parameter: -
    Summary: print all the money as manager checks the money earned by each clerk
    Return value: void */

void printMoney();

/*CL: Parameter: -
    Summary: manager code, interrupts are disabled
    Return value: void*/

void Manager();

/*CL: Parameter: -
    Summary: check if customers are done, i.e. are all attributes set to true
    Return value: void*/

int customersAreAllDone();

#endif /* PASSPORTOFFICE_H */

