typedef struct Node
{ /* linked list consists of nodes*/
  int messageTTL; //only saved on the head node
  int index; //saved on all nodes, but only needed on the head node
  char *key;
  char *value;
  struct Node *next;
} node;