#include "llstruct.h"
#include "euclidean.h"
#include "include.h"

void addNode(char *key, char *value);
void createNewList();
void decrementTTL(node **head);
node *findNode(node *head, char *key);
void freeAll();
void freeList(node **head);
char *getForwardedLL(node *head);
int getIntValue(node *head, char *key);
int getListIndex(node *head);
char *getStringValue(node *head, char *key);
int hasKey(node *head, char *key);
bool hasPair(node *head, char *key, char *value);
void initLLA();
bool isCurrentListDuplicate(node *head);
int isCurrentListValid();
bool isDuplicateLists(node *head1, node *head2);
void prepareToForward(int myLocation, int msgTTL);
void printList(node *head);
void setIsForwardMsg(int i);
int tryGetIntValue(node *head, char *key);
void updateSendPath(int portNumber);
void updateValue(node *head, char *key, char *newValue);
int verifyVersion(node *head);