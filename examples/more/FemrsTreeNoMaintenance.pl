#name "Contention-friendly Binary Search Tree, no Maintenance"


struct Node {
    thread_t lock;
	data_t val;
	Node* left;
	Node* right;
    bool del;
    bool rem;
}

Node* Root;


def @contains(Node* node, data_t key) {
    !node->del && node->val == key
}

def @outflow[left](Node* node, data_t key) {
    key < node->val && node->left != NULL
}

def @outflow[right](Node* node, data_t key) {
    node->val < key && node->right != NULL
}

def @invariant[local](Node* x) {
    x->_flow == 0
}

def @invariant[shared](Node* x) {
    Root != NULL
 && Root->val == MIN
 && Root->del == false
 && Root->rem == false
 && Root->_flow != 0
 && [MIN, MAX] in Root->_flow
 && x->val == MIN ==> x == Root

 && x->_flow != 0 ==> [x->val, x->val] in x->_flow
 // && (!x->rem && x->lock == UNLOCKED) ==> x->_flow != 0
 // && !x->del ==> x->_flow != 0
 && !x->rem ==> x->_flow != 0
 && !x->del ==> !x->rem
 && x->rem ==> x->del
}


void __init__() {
	Root = malloc;
    Root->val = MIN;
    Root->left = NULL;
    Root->right = NULL;
    Root->del = false;
    Root->rem = false;
}


inline <Node*, Node*> locate(data_t key) {
	Node* pred, curr;

    pred = Root;
    curr = Root;
    do {
        pred = curr;
        if (pred->val < key) curr = pred->right;
        else curr = pred->left;
    } while (curr != NULL && curr->val != key);
    return <pred, curr>;
}


inline void lockWithHint(Node* ptr) {
    atomic {
        __lock__(ptr->lock);
        __unlock__(ptr->lock);
        choose { assume(ptr->rem == true); }{ assume(ptr->rem == false); }
        // choose { assume(ptr->del == true); }{ assume(ptr->del == false); }
        __lock__(ptr->lock);
    }
}


bool contains(data_t key) {
	Node* pred, curr;
	<pred, curr> = locate(key);
    if (curr == NULL) return false;
    else if (curr->del) return false;
    else return true;
}


bool add(data_t key) {
    while (true) {
        Node* pred, curr;
        bool ret;
        <pred, curr> = locate(key);

        if (curr != NULL) {
            lockWithHint(curr);
            if (!curr->rem) {
                if (curr->del) {
                    curr->del = false;
                    __unlock__(curr->lock);
                    return true;
                } else {
                    curr->del = false;
                    __unlock__(curr->lock);
                    return false;
                }
            }
        } else {
            lockWithHint(pred);
            if (!pred->rem) {
                if (key < pred->val && pred->left == NULL) {
                    Node* entry;
                    entry = malloc;
                    entry->val = key;
                    entry->left = NULL;
                    entry->right = NULL;
                    entry->del = false;
                    entry->rem = false;

                    pred->left = entry;
                    __unlock__(pred->lock);
                    return true;
                }
                if (key > pred->val && pred->right == NULL) {
                    Node* entry;
                    entry = malloc;
                    entry->val = key;
                    entry->left = NULL;
                    entry->right = NULL;
                    entry->del = false;
                    entry->rem = false;

                    pred->right = entry;
                    __unlock__(pred->lock);
                    return true;
                }
                __unlock__(pred->lock);
            }
        }
    }
}


bool remove(data_t key) {
    while (true) {
        Node* pred, curr;
        bool ret;
        <pred, curr> = locate(key);

        if (curr == NULL) return false;
        lockWithHint(curr);
        if (!curr->rem) {
            if (!curr->del) {
                curr->del = true;
                __unlock__(curr->lock);
                return true;
            } else {
                curr->del = true;
                __unlock__(curr->lock);
                return false;
            }
        }
    }
}
