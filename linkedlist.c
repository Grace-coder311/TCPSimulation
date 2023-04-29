#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linkedlist.h"

// knows where in the list of pointers to ll of nodes that another list can be added
int ll_index = 0;
// this list is a list of heads to individual ll's, need a reference
node *head = NULL;
static node *current = NULL;
/* linked list nodes used for traversal - allocate mem for the ptrs */
node **llarray = NULL;
static int isForwardMsg = 0;
static int forwardedIndex = 0;
//TODO - fill deleted spaces with new LL's by keeping a list of their indexes and filling those spots

void initLLA()
{
  int i;
  llarray = (node **)malloc(MAX_SERVERS * sizeof(node *)); /* ptr to head ptr for each server's associated linked list*/
  for (i = 0; i < MAX_SERVERS; i++)
    llarray[i] = NULL;
}

int verifyVersion(node *head)
{
  node *r = findNode(head, "version");
  if (r != NULL && atoi(r->value) == VERSION)
    return 1;

  return 0;
}

/*only used to get integer values from specified key in a ll*/
int getIntValue(node *head, char *key)
{
  return atoi(getStringValue(head, key));
}

/*returns null if the value doesn't exit*/
int tryGetIntValue(node *head, char *key)
{
  char value[1000];

  if (!hasKey(head, key))
  {
    return 0;
  }

  node *r = findNode(head, key);
  strcpy(value, r->value);

  return atoi(strdup(value));
}

char *getStringValue(node *head, char *key)
{
  char value[1000];

  if (!hasKey(head, key))
  {
    printf("key: %s\n", key);
    perror(">:( no value if no key!");
  }
  node *r = findNode(head, key);
  strcpy(value, r->value);

  return strdup(value);
}

int hasKey(node *head, char *key)
{
  if (findNode(head, key) != NULL)
    return 1;

  return 0;
}

bool hasPair(node *head, char *key, char *value)
{
  char *actualValue;
  bool isExisting = false;

  if (hasKey(head, key))
  {
    actualValue = getStringValue(head, key);
    isExisting = strcmp(value, actualValue) == 0;
  }

  return isExisting;
}

void updateValue(node *head, char *key, char *newValue)
{
  if (hasKey(head, key))
  {
    node *r = findNode(head, key);
    r->value = strdup(newValue);
  }
  else
  {
    perror("you cannot update a key that doesn't exist");
  }
}

/*handle adding a new ll to the list of pointers - invalid ll's will be removed */
void createNewList()
{ // a new challenger approaches in the form of a linked list
  head = NULL;
  current = NULL;

  ll_index++;
}

void addNode(char *key, char *value)
{
  node *link;

  // allocate dynamic memory
  link = malloc(sizeof(node)); 
  link->key = (char *) malloc(100);
  link->value = (char *) malloc(100);
  link->next = (node *) sizeof(node *);

  // index should be on every node to know where it's stored on the drone
  link->index= ll_index;

  // create a link and assign the key-value pair
  link->key = strdup(key);
  link->value = strdup(value);
  link->next = NULL;
  
  if (head == NULL || !head)
  {
    link->messageTTL = MSG_TTL;
    head = link;
    current = link;
    (llarray)[ll_index] = head; /*pts to each ll head should be stored in the array*/
  }
  else
  {
    if (hasKey(head, key))
    {
      printList(head);
      printf("attempted to add key '%s' with value '%s'\n", key, value);
      perror("duplicate keys are not allowed!\n");
      exit(1);
    }

    link->messageTTL = -1;
    current->next = link;
    current = current->next;
  }
}

void decrementTTL(node **head)
{
  (*head)->messageTTL--;

  if((*head)->messageTTL < 1)
  {
    freeList(head);
  }
}

// display the given linked list
void printList(node *head)
{
  node *ptr = head;

  printf("\n%*s | %*s\n", -13, "Key", -35, "Value");
  // start from the beginning
  while (ptr != NULL)
  {
    printf("%*s | %*s\n", -13, ptr->key, -35, ptr->value);
    ptr = ptr->next;
  }
  printf("_____________________________\n");
}

node *findNode(node *head, char *key)
{
  node *current = head;

  if (head == NULL || !head) // empty list had no nodes
    return NULL;

  while (current != NULL)
  {
    if (strcmp(current->key, key) == 0)
      return current;
    current = current->next; // move to the next one
  }

  return NULL; // last node found, so it doesn't exist
}

int getListIndex(node *head)
{
  if(head == NULL)
  {
    perror("no index value for null list");
    exit(1);
  }

  return head->index;
}

void freeAll()
{
  int i;

  /* free memory allocated to each node instance */
  for (i = 0; i < MAX_SERVERS; i++)
  {
    if ((llarray)[i] == NULL)
      continue;
    
    freeList(&llarray[i]); // free that sweet-sweet memory!
  }

  if (llarray)
    free(llarray);
}

/* free all the memory allocatted for the given linked list in llarray*/
/* delete the given LL from the list and sets it to NULL */
void freeList(node **head)
{
  node *temp;

  if((*head) == NULL)
  {
    perror("NULL is an invalid list");
    exit(1);
  }
  
  while ((*head) != NULL)
  {
    temp = (*head);
    (*head) = (*head)->next;
    free(temp->key);
    free(temp->value);
    temp->next = NULL; // ! ref stored in head, so this can be set to null
    free(temp);
  }

  (*head) = NULL; //don't be pointing to garbage
}

int isCurrentListValid()
{
  int isValid = 1;

  isValid = head != NULL && verifyVersion(head) && hasKey(head, "location") &&
            hasKey(head, "toPort") && hasKey(head, "fromPort") && hasKey(head, "msg") && hasKey(head, "TTL");

  return isValid;
}

/*convert the current list back into a string msg*/
char *getForwardedLL(node *head)
{
  char msg[1000] = "";
  char newPair[500];

  node *ptr = head;

  if (!isForwardMsg)
    return NULL;
  else // only need to extract this once, then no more
    isForwardMsg = 0;

  if (head == NULL || !head) // empty list had no nodes
  {
    perror("can't forward from a non-existant LL");
    exit(1);
  }

  while (ptr != NULL)
  {
    memset(newPair, 0, 500);
    snprintf(newPair, 500, "%s:%s", ptr->key, ptr->value);
    strcat(msg, newPair);

    if (ptr->next != NULL)
      strcat(msg, " ");

    ptr = ptr->next; // move to the next one
  }

  return strdup(msg); // last node found, so it doesn't exist
}

void prepareToForward(int myLocation, int msgTTL)
{
  const int SIZE = 15;
  char newValue[SIZE];

  isForwardMsg = 1;
  forwardedIndex = ll_index; // will generally be the ll_index - 1, but this makes things easier

  snprintf(newValue, SIZE, "%d", (msgTTL - 1));
  updateValue(head, "TTL", newValue);
  snprintf(newValue, SIZE, "%d", myLocation);
  updateValue(head, "location", newValue);

  //going to get the send path to exlude those from it

}

/*send-path key will store the orginal port and all the hops along the way (not including the toPort)*/
void updateSendPath(int portNumber)
{
  char currentPort[100];
  char newValue[100];

  strcpy(newValue, getStringValue(head, "send-path"));
  snprintf(currentPort, 100, ",%d", portNumber);
  strcat(newValue, currentPort);

  if (portNumber != getIntValue(head, "toPort"))
  {
    updateValue(head, "send-path", newValue);
  }
}

void setIsForwardMsg(int i)
{
  isForwardMsg = i;
}

/*check if two lists are duplicate*/
bool isDuplicateLists(node *head1, node *head2)
{
  int seqNum1, seqNum2, toPort1, toPort2, fromPort1, fromPort2;
  bool isIdentical = false;
  bool isSameSeqNum, isSameToPort, isSameFromPort;

  if(getListIndex(head1) == getListIndex(head2))
  { //comparing the same list doesn't count because they are the same copy
    return false;
  }

  seqNum1 = tryGetIntValue(head1, "seqNumber");
  seqNum2 = tryGetIntValue(head2, "seqNumber");

  toPort1 = tryGetIntValue(head1, "toPort");
  toPort2 = tryGetIntValue(head2, "toPort");

  fromPort1 = tryGetIntValue(head1, "fromPort");
  fromPort2 = tryGetIntValue(head2, "fromPort");

  // they should all be non-null integer values before comparison
  // if everything is null and equals we don't want that!
  if(seqNum1 && seqNum2 && toPort1 && toPort2 && fromPort1 && fromPort2)
  {
    isSameSeqNum = seqNum1 == seqNum2;
    isSameToPort = toPort1 == toPort2;
    isSameFromPort = fromPort1 == fromPort2;
    isIdentical = isSameSeqNum && isSameToPort && isSameFromPort;
  }
  
  return isIdentical;
}

/*check if the current list already exists in the list of LLs*/
bool isCurrentListDuplicate(node *head)
{
  bool isDuplicate = false;
  int i;

  /* check each node to see if it matches the given ll */
  for (i = 0; i < ll_index; i++)
  {
    if (isDuplicate)
      break;

    /* want to avoid the copy being compared to itself*/
    if ((llarray)[i] == NULL)
      continue;

    isDuplicate = isDuplicateLists((llarray)[i], head);
  }

  return isDuplicate ;
}
