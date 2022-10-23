
#include <initguid.h>
#include <ntifs.h>
#include <ntdef.h>
#include <fltKernel.h>
#include <wdm.h>
#include <wsk.h>
#include <dontuse.h>
#include <suppress.h>
#include "nlwfse.h"
#include "nllib.h"
#include "avlTree.h"

extern NLWFSE_DATA wfse;

extern int debugCache;

void wfseAvlInsert(Pwfse_AVL_TREE t, 
                   void *data,
                   PPAGED_LOOKASIDE_LIST Lookaside,
                   PFLT_INSTANCE Instance)
{
    Pwfse_QUERY_CACHE n = (Pwfse_QUERY_CACHE)data;
    Pwfse_AVL_TREE avl;
    PNLWFSE_VOLUME vol;
    int idx;

    //Get volume idx
    for (idx = 0; idx < WFSE_MAX_VOLUMES; idx++) {
        vol = &wfse.Gv[idx];
        if (wfse.Gv[idx].Instance == Instance) {
            break;
        }
    } 
  
    if (t->fileQueryCache == NULL) {
        avl = wfse.Gv[idx].avl[t->TreeIdx];            
        t->fileQueryCache = wfseAvlNodeCreate(data, NULL, Lookaside);
        t->cacheCnt++;  //cache cnt in individual cache
        ExAcquireFastMutex(&wfse.gLock);
	wfse.MaxCacheCnt++; // global cache cnt
        ExReleaseFastMutex(&wfse.gLock);
        wfse.Gv[idx].cacheCnt++; // volume cache cnt

        if (debugCache) {
            DbgPrint("wfseAvlInsert: init gv table idx %d avltree %p root node %p cache cnt %d vol cache cnt %d maxCachecnt %d \n", idx, t, t->fileQueryCache, t->cacheCnt, wfse.Gv[idx].cacheCnt, wfse.MaxCacheCnt);
        }

    } else {
        PwfseAvlNode node = t->fileQueryCache;
        while (node != NULL) {
            if (Tree_KeyCompare(data, node) < 0) {
                PwfseAvlNode left = node->left;
                if (left == NULL) {
                    node->left = wfseAvlNodeCreate(data, node, Lookaside);
                    Tree_InsertBalance(t, node, -1);
                    t->cacheCnt++;  //cache cnt in individual cache
                    ExAcquireFastMutex(&wfse.gLock);
	            wfse.MaxCacheCnt++; // global cache cnt
                    ExReleaseFastMutex(&wfse.gLock);
                    wfse.Gv[idx].cacheCnt++; // volume cache cnt
                    if (debugCache) {
                        DbgPrint("wfseAvlInsert: node less %p \n", node->left );
                        DbgPrint("wfseAvlInsert: idx %d cache cnt %d volume cnt %d global cnt %d \n", idx, t->cacheCnt, wfse.Gv[idx].cacheCnt, wfse.MaxCacheCnt);
                    }
                    return;
                } else {
                    node = left;
                }
             } else if (Tree_KeyCompare(data, node) > 0) {
                PwfseAvlNode right = node->right;
                if (right == NULL) {
                    node->right = wfseAvlNodeCreate(data, node, Lookaside);
                    Tree_InsertBalance(t, node, 1);
                    t->cacheCnt++;
                    ExAcquireFastMutex(&wfse.gLock);
		    wfse.MaxCacheCnt++;
                    ExReleaseFastMutex(&wfse.gLock);
                    wfse.Gv[idx].cacheCnt++;
                    if (debugCache) {
                        DbgPrint("wfseAvlInsert: node greater %p \n", node->right );
                        DbgPrint("wfseAvlInsert: idx %d cache cnt %d volume cnt %d global cnt %d \n", idx, t->cacheCnt, wfse.Gv[idx].cacheCnt, wfse.MaxCacheCnt);
                    }
                    return;
                } else {
                    node = right;
                }
            } else {
                node->key = n->key;
                node->allowedAccess = n->allowedAccess;
                node->denyRename = n->denyRename;
                node->reqTime = n->reqTime;
             
                if (debugCache) {
                    DbgPrint("wfseAvlInsert: node equal %p \n", node );
                    DbgPrint("wfseAvlInsert: idx %d cache cnt %d volume cnt %d global cnt %d \n", idx, t->cacheCnt, wfse.Gv[idx].cacheCnt, wfse.MaxCacheCnt);
                }

                return;
            }
        }
    }
}

void wfseAvlDeleteNode(Pwfse_AVL_TREE t, PwfseAvlNode node, PFLT_INSTANCE Instance)
{
  PwfseAvlNode left = node->left;
  PwfseAvlNode right = node->right;
  PwfseAvlNode DeleteNode = node;
  int idx;

   for (idx = 0; idx < WFSE_MAX_VOLUMES; idx++) {
      if (wfse.Gv[idx].Instance == Instance) {
            break;
      }
   } 

  if (left == NULL) {
    if (right == NULL) {
      if (node == t->fileQueryCache) {
        t->fileQueryCache = NULL;
      }
      else {
        PwfseAvlNode parent = node->parent;
        if (parent->left == node) {
          parent->left = NULL;
          Tree_DeleteBalance(t, parent, 1);
        }
        else {
          parent->right = NULL;
          Tree_DeleteBalance(t, parent, -1);
        }
      }
    }
    else {
      Tree_Replace(node, right);
      Tree_DeleteBalance(t, node, 0);
      DeleteNode = right;
    }
  }
  else if (right == NULL) {
    Tree_Replace(node, left);
    Tree_DeleteBalance(t, node, 0);
    DeleteNode = left;
  }
  else {
    PwfseAvlNode successor = right;
    if (successor->left == NULL) {
      PwfseAvlNode parent = node->parent;
      successor->parent = parent;
      successor->left = left;
      successor->balance = node->balance;

      if (left != NULL) {
        left->parent = successor;
      }
      if (node == t->fileQueryCache) {
        t->fileQueryCache = successor;
      }
      else {
        if (parent->left == node) {
          parent->left = successor;
        }
        else {
          parent->right = successor;
        }
      }
      Tree_DeleteBalance(t, successor, -1);
    }
    else {
      while (successor->left != NULL) {
        successor = successor->left;
      }
      PwfseAvlNode parent = node->parent;
      PwfseAvlNode successorParent = successor->parent;
      PwfseAvlNode successorRight = successor->right;

      if (successorParent->left == successor) {
        successorParent->left = successorRight;
      }
      else {
        successorParent->right = successorRight;
      }

      if (successorRight != NULL) {
        successorRight->parent = successorParent;
      }

      successor->parent = parent;
      successor->left = left;
      successor->balance = node->balance;
      successor->right = right;
      right->parent = successor;

      if (left != NULL) {
        left->parent = successor;
      }

      if (node == t->fileQueryCache) {
        t->fileQueryCache = successor;
      }
      else {
        if (parent->left == node) {
          parent->left = successor;
        }
        else {
          parent->right = successor;
        }
      }
      Tree_DeleteBalance(t, successorParent, 1);
    }
  }
  t->cacheCnt--;   //single tree cache count in instance
  ExAcquireFastMutex(&wfse.gLock);
  wfse.MaxCacheCnt--; // global cache count
  ExReleaseFastMutex(&wfse.gLock);
  wfse.Gv[idx].cacheCnt--; // total cache count in instance
    if (debugCache) {
        DbgPrint("wfseAvlDeleteNode: Free node %p \n", DeleteNode );
        DbgPrint("wfseAvlDeleteNode: idx %d cache cnt %d volume cnt %d global cnt %d \n", idx, t->cacheCnt, wfse.Gv[idx].cacheCnt, wfse.MaxCacheCnt);
    }
  ExFreeToPagedLookasideList(&t->LookasideField, DeleteNode);

}

int Tree_KeyCompare(void *a1, void *a2)
{
  Pwfse_QUERY_CACHE nd1 = (Pwfse_QUERY_CACHE)a1;
  PwfseAvlNode nd2 = (PwfseAvlNode)a2;

  if (nd1->key < nd2->key) {
    return -1;
  }
  else if (nd1->key > nd2->key) {
    return +1;
  }
  else {
    return 0;
  }
}

PwfseAvlNode wfseAvlSearchNode(Pwfse_AVL_TREE t, void *data)
{
  PwfseAvlNode node = t->fileQueryCache;

  while (node != NULL) {
    if (Tree_KeyCompare(data, node) < 0) {
      node = node->left;
    }
    else if (Tree_KeyCompare(data, node) > 0) {
      node = node->right;
    }
    else {
        //TODO: update timestamp
        if (debugCache) {
            DbgPrint("wfseAvlSearchNode: found match node %p key %x \n", node, node->key);
        }
      return node;
    }
  }

  return NULL;
}

VOID wfseAvlFreeNode(PwfseAvlNode node, PPAGED_LOOKASIDE_LIST Lookaside)
{

  if (node) {
      if (node->left)
          wfseAvlFreeNode(node->left, Lookaside);
      if (node->right)
          wfseAvlFreeNode(node->right, Lookaside);
      if (node != NULL) {
         if (debugCache) {
             DbgPrint("wfseAvlFreeNode: Free avl node %p \n", node);
         }

          ExFreeToPagedLookasideList(Lookaside, node); 
      } else {

         if (debugCache) {
            DbgPrint("wfseAvlFreeNode: node is null\n");
         }
      }
  }

  return;
}

void Tree_InsertBalance(Pwfse_AVL_TREE t, PwfseAvlNode node, int balance)
{
  if (debugCache) {
      DbgPrint("Tree_InsertBalance: called node %p \n", node );
  }

  while (node != NULL) {
    balance = (node->balance += balance);
    if (balance == 0) {
      return;
    }
    else if (balance == -2) {
      if (node->left->balance == -1) {
        Tree_RotateRight(t, node);
      }
      else {
        Tree_RotateLeftRight(t, node);
      }
      return;
    }
    else if (balance == 2) {
      if (node->right->balance == 1) {
        Tree_RotateLeft(t, node);
      }
      else {
        Tree_RotateRightLeft(t, node);
      }
      return;
    }
    PwfseAvlNode parent = node->parent;
    if (parent != NULL) {
      balance = (parent->left == node) ? -1 : 1;
    }
    node = parent;
  }
}

void Tree_DeleteBalance(Pwfse_AVL_TREE t, PwfseAvlNode node, int balance)
{
  if (debugCache) {
      DbgPrint("Tree_DeleteBalance: called node %p \n", node );
  }
  while (node != NULL) {
    balance = (node->balance += balance);

    if (balance == -2) {
      if (node->left->balance <= 0) {
        node = Tree_RotateRight(t, node);

        if (node->balance == 1) {
          return;
        }
      }
      else {
        node = Tree_RotateLeftRight(t, node);
      }
    }
    else if (balance == 2) {
      if (node->right->balance >= 0) {
        node = Tree_RotateLeft(t, node);

        if (node->balance == -1) {
          return;
        }
      }
      else {
        node = Tree_RotateRightLeft(t, node);
      }
    }
    else if (balance != 0) {
      return;
    }

    PwfseAvlNode parent = node->parent;

    if (parent != NULL) {
      balance = (parent->left == node) ? 1 : -1;
    }

    node = parent;
  }
}

void Tree_Replace(PwfseAvlNode target, PwfseAvlNode source)
{
  if (debugCache) {
      DbgPrint("Tree_Replace: called source %p target %p \n", source, target );
  }

  PwfseAvlNode left = source->left;
  PwfseAvlNode right = source->right;

  target->balance = source->balance;
  target->key = source->key;
  target->allowedAccess = source->allowedAccess;
  target->denyRename = source->denyRename;
  target->reqTime = source->reqTime;
  target->left = left;
  target->right = right;

  if (left != NULL) {
    left->parent = target;
  }

  if (right != NULL) {
    right->parent = target;
  }
}

PwfseAvlNode Tree_RotateLeft(Pwfse_AVL_TREE t, PwfseAvlNode node)
{
  if (debugCache) {
      DbgPrint("Tree_RotateLeft: called node %p \n", node );
  }

  PwfseAvlNode right = node->right;
  PwfseAvlNode rightLeft = right->left;
  PwfseAvlNode parent = node->parent;

  right->parent = parent;
  right->left = node;
  node->right = rightLeft;
  node->parent = right;

  if (rightLeft != NULL) {
    rightLeft->parent = node;
  }

  if (node == t->fileQueryCache) {
    t->fileQueryCache = right;
  }
  else if (parent->right == node) {
    parent->right = right;
  }
  else {
    parent->left = right;
  }

  right->balance--;
  node->balance = -right->balance;

  return right;
}

PwfseAvlNode Tree_RotateRight(Pwfse_AVL_TREE t, PwfseAvlNode node)
{
  if (debugCache) {
      DbgPrint("Tree_RotateRight: called node %p \n", node);
  }

  PwfseAvlNode left = node->left;
  PwfseAvlNode leftRight = left->right;
  PwfseAvlNode parent = node->parent;

  left->parent = parent;
  left->right = node;
  node->left = leftRight;
  node->parent = left;

  if (leftRight != NULL) {
    leftRight->parent = node;
  }

  if (node == t->fileQueryCache) {
    t->fileQueryCache = left;
  }
  else if (parent->left == node) {
    parent->left = left;
  }
  else {
    parent->right = left;
  }

  left->balance++;
  node->balance = -left->balance;

  return left;
}

PwfseAvlNode Tree_RotateLeftRight(Pwfse_AVL_TREE t, PwfseAvlNode node)
{
  if (debugCache) {
      DbgPrint("Tree_RotateLeftRight: called node %p \n", node);
  }

  PwfseAvlNode left = node->left;
  PwfseAvlNode leftRight = left->right;
  PwfseAvlNode parent = node->parent;
  PwfseAvlNode leftRightRight = leftRight->right;
  PwfseAvlNode leftRightLeft = leftRight->left;

  leftRight->parent = parent;
  node->left = leftRightRight;
  left->right = leftRightLeft;
  leftRight->left = left;
  leftRight->right = node;
  left->parent = leftRight;
  node->parent = leftRight;

  if (leftRightRight != NULL) {
    leftRightRight->parent = node;
  }

  if (leftRightLeft != NULL) {
    leftRightLeft->parent = left;
  }

  if (node == t->fileQueryCache) {
    t->fileQueryCache = leftRight;
  }
  else if (parent->left == node) {
    parent->left = leftRight;
  }
  else {
    parent->right = leftRight;
  }

  if (leftRight->balance == 1) {
    node->balance = 0;
    left->balance = -1;
  }
  else if (leftRight->balance == 0) {
    node->balance = 0;
    left->balance = 0;
  }
  else {
    node->balance = 1;
    left->balance = 0;
  }

  leftRight->balance = 0;

  return leftRight;
}

PwfseAvlNode Tree_RotateRightLeft(Pwfse_AVL_TREE t, PwfseAvlNode node)
{
  if (debugCache) {
      DbgPrint("Tree_RotateRightLeft: called node %p \n", node);
  }

  PwfseAvlNode right = node->right;
  PwfseAvlNode rightLeft = right->left;
  PwfseAvlNode parent = node->parent;
  PwfseAvlNode rightLeftLeft = rightLeft->left;
  PwfseAvlNode rightLeftRight = rightLeft->right;

  rightLeft->parent = parent;
  node->right = rightLeftLeft;
  right->left = rightLeftRight;
  rightLeft->right = right;
  rightLeft->left = node;
  right->parent = rightLeft;
  node->parent = rightLeft;

  if (rightLeftLeft != NULL) {
    rightLeftLeft->parent = node;
  }

  if (rightLeftRight != NULL) {
    rightLeftRight->parent = right;
  }

  if (node == t->fileQueryCache) {
    t->fileQueryCache = rightLeft;
  }
  else if (parent->right == node) {
    parent->right = rightLeft;
  }
  else {
    parent->left = rightLeft;
  }

  if (rightLeft->balance == -1) {
    node->balance = 0;
    right->balance = 1;
  }
  else if (rightLeft->balance == 0) {
    node->balance = 0;
    right->balance = 0;
  }
  else {
    node->balance = -1;
    right->balance = 0;
  }

  rightLeft->balance = 0;

  return rightLeft;
}

BOOLEAN wfseAvlTimeoutNode(
    Pwfse_AVL_TREE t, 
    void *data,
    PPAGED_LOOKASIDE_LIST Lookaside,
    PFLT_INSTANCE Instance)
{
    CSHORT min = 0;
    BOOLEAN inserted = FALSE;
    Pwfse_QUERY_CACHE n = (Pwfse_QUERY_CACHE)data;
    TIME_FIELDS ctime, qctime;
    LARGE_INTEGER curSystemTime;
    PwfseAvlNode node = t->fileQueryCache;
    CSHORT cacheTime = WFSE_QUERY_CACHE_TIMEOUT;
    KeQuerySystemTime(&curSystemTime);
    RtlTimeToTimeFields(&curSystemTime, &ctime);

    while (node != NULL) {
        RtlTimeToTimeFields(&node->reqTime, &qctime);
        if (ctime.Hour == qctime.Hour) {
            min = ctime.Minute - qctime.Minute;
        } else if ((ctime.Hour > qctime.Hour) ||
	   (qctime.Hour == 23)) {
            min = ctime.Minute + (60 - qctime.Minute);
        } 

        if (min > cacheTime) {
            wfseAvlDeleteNode(t, node, Instance);
            wfseAvlInsert(t, data, Lookaside, Instance);

    if (debugCache) {
        DbgPrint("wfseAvlTimeoutNode: timeout insert node key %x allowedAccess %x \n", n->key, n->allowedAccess);
    }

            inserted = TRUE;
            break;
        }

        if (Tree_KeyCompare(data, node) < 0) {
            node = node->left;
        } else if (Tree_KeyCompare(data, node) > 0) {
            node = node->right;
        } 
    }

    return inserted;
}

PwfseAvlNode wfseAvlNodeCreate(
    void *data,
    PwfseAvlNode parent,
    PPAGED_LOOKASIDE_LIST Lookaside)
{
    PwfseAvlNode n;
    Pwfse_QUERY_CACHE q;

    n = (PwfseAvlNode) ExAllocateFromPagedLookasideList(Lookaside);
    q = (Pwfse_QUERY_CACHE) data;
    if (debugCache) {
        DbgPrint("wfseAvlNodeCreate: Allocated node %p \n", n );
    }

    if (n) {
        n->parent = parent;
        n->left = NULL;
        n->right = NULL;
        n->key = q->key;
        n->allowedAccess = q->allowedAccess;
        n->denyRename = q->denyRename;
        n->reqTime = q->reqTime;
        n->balance = 0;
    } else {
        DbgPrint("wfseAvlNodeCreate: memory allocation failed \n");
    }

  return n;
}

