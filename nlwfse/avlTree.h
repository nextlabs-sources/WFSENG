
#ifndef __AVLTREE_H__
#define __AVLTREE_H__

void wfseAvlInsert(Pwfse_AVL_TREE t, 
                   void *data,
                   PPAGED_LOOKASIDE_LIST Lookaside,
                   PFLT_INSTANCE Instance);
void wfseAvlDeleteNode(Pwfse_AVL_TREE t,
                       PwfseAvlNode node,
                       PFLT_INSTANCE Instance);
PwfseAvlNode wfseAvlSearchNode(Pwfse_AVL_TREE t, void *data);
VOID wfseAvlFreeNode(PwfseAvlNode node, PPAGED_LOOKASIDE_LIST Lookaside);
BOOLEAN wfseAvlTimeoutNode(Pwfse_AVL_TREE t, void *data, PPAGED_LOOKASIDE_LIST Lookaside, PFLT_INSTANCE Instance);
//void *wfseAvlGetData(Node n);

void Tree_InsertBalance(Pwfse_AVL_TREE t, PwfseAvlNode node, int balance);
void Tree_DeleteBalance(Pwfse_AVL_TREE t, PwfseAvlNode node, int balance);
PwfseAvlNode Tree_RotateLeft(Pwfse_AVL_TREE t, PwfseAvlNode node);
PwfseAvlNode Tree_RotateRight(Pwfse_AVL_TREE t, PwfseAvlNode node);
PwfseAvlNode Tree_RotateLeftRight(Pwfse_AVL_TREE t, PwfseAvlNode node);
PwfseAvlNode Tree_RotateRightLeft(Pwfse_AVL_TREE t, PwfseAvlNode node);
void Tree_Replace(PwfseAvlNode target, PwfseAvlNode source);
int Tree_KeyCompare(void *a1, void *a2);
PwfseAvlNode wfseAvlNodeCreate(void *data, PwfseAvlNode parent, PPAGED_LOOKASIDE_LIST Lookaside);


#endif
