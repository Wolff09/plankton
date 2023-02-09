#name "Logical Ordering Tree (wip)"


//*******************************************************//
//************************* API *************************//
//*******************************************************//

struct Node {
    thread_t treeLock;
    thread_t listLock;
    data_t key;
    Node* left;
    Node* right;
    Node* parent;
    Node* pred;
    Node* succ;
    bool mark;
}

Node* Min;
Node* Max;


bool contains(data_t k) {
    Node* y, foo;
    y = traverse(k);
    @join; // noop, makes proof faster

    while (k < y->key) {
        Node* x;
        x = y->pred;
        y = x;
    }
    while (y->mark) {
        Node* x;
        x = y->pred;
        y = x;
    }
    while (y->key < k) {
        Node* z;
        z = y->succ;
        y = z;
    }

    if (y->key == k) return true;
    else return false;
}


bool remove(data_t k) {
    Node* x, y, z, c;
    bool flag;
    <x, c, y> = locate(k);

    assertFlow(k, y);
    if (y->key != k) {
        __unlock__(x->listLock);
        return false;
    }

    __lock__(y->listLock);
    flag = prepareTreeDeletion(y);
    z = y->succ;
    y->mark = true;
    z->pred = x;
    x->succ = z; // logical deletion
    __unlock__(y->listLock);
    __unlock__(x->listLock);
    performTreeDeletion(y, flag);
    return true;
}


bool insert(data_t k) {
    Node* x, y, z, p, c;
    <x, c, z> = locate(k);
    
    assertFlow(k, z);
    if (z->key == k) {
        __unlock__(x->listLock);
        return false;
    }

    p = prepareTreeInsertion(x, z, c);
    y = malloc;
    y->key = k;
    y->mark = false;
    y->left = NULL;
    y->right = NULL;
    y->parent = p;
    y->pred = x;
    y->succ = z;
    // __lock__(y->treeLock);

    assertFlow(k, x);
    x->succ = y;
    z->pred = y;
    __unlock__(x->listLock);
    performTreeInsertion(y, p);
    return true;
}


//*******************************************************//
//*********************** HELPERS ***********************//
//*******************************************************//

inline Node* traverse(data_t k) {
    Node* y;
    y = Max;
    while (true) {
        Node* c;
        if (y->key == k) return y;
        if (k < y->key) c = y->left;
        else c = y->right;
        if (c == NULL) return y;
        y = c;
    }
}


inline <Node*, Node*, Node*> locate(data_t k) {
    while (true) {
        Node* x, y, z;
        y = traverse(k);
        if (y->key < k) x = y;
        else x = y->pred;
        @join; // noop, makes proof faster
        __lock__(x->listLock);
        z = x->succ;
        if (x->key < k && k <= z->key && !x->mark) {
            return <x, y, z>;
        }
        __unlock__(x->listLock);
    }
}


//*******************************************************//
//********************** INVARIANT **********************//
//*******************************************************//

def @acyclicity { effective }

def @contains(Node* x, data_t k) { x->key == k }

def @outflow[left](Node* x, data_t k) { false }
def @outflow[right](Node* x, data_t k) { false }
def @outflow[parent](Node* x, data_t k) { false }
def @outflow[pred](Node* x, data_t k) { false }
def @outflow[succ](Node* x, data_t k) { x->key < k }

def @invariant[local](Node* x) {
    x->_flow == 0
}

def @invariant[shared](Node* x) {
    // Min
    Min != NULL
 && Min->key == MIN
 && Min->mark == false
 && x->key == MIN ==> x == Min
    // Max
 && Max != NULL
 && Max->key == MAX
 && Max->mark == false
 && x->key == MAX ==> x == Max
    // flow
 && [MIN, MAX] in Min->_flow
 && x->_flow != 0 ==> [x->key, MAX] in x->_flow
 && !x->mark ==> x->_flow != 0
    // list
 && x->succ == NULL ==> x == Max
 && x->pred == NULL ==> x == Min
 && x == Max ==> x->succ == NULL
 && x == Min ==> x->pred == NULL
    // tree
 && x->key != MIN ==> x->parent != NULL
}

def @invariant[pairwise](Node* x, Node* y) {
    x->succ == y ==> x->key < y->key
 && y->pred == x ==> x->key < y->key
 && x->left == y ==> y->key < x->key  // plankton fails to show these
 && x->right == y ==> x->key < y->key // plankton fails to show these
 && x->right == y ==> y->key != MIN
 && x->right == y ==> y->key != MAX
 && x->left == y ==> y->key != MIN
 && x->left == y ==> y->key != MAX
}


//*******************************************************//
//************************ INIT *************************//
//*******************************************************//

void __init__() {
    Max = malloc;
    Min = malloc;
    Min->key = MAX;
    Min->left = NULL;
    Min->right = NULL;
    Min->parent = NULL;
    Min->pred = NULL;
    Min->succ = Max;
    Min->mark = false;
    Max->key = MIN;
    Max->left = NULL;
    Max->right = NULL;
    Max->parent = Min;
    Max->pred = Min;
    Max->succ = NULL;
    Max->mark = false;
}


//*******************************************************//
//********************* TREE STUBS **********************//
//*******************************************************//

inline void rebalanceIns(Node* p) {
    // do nothing
}

inline void rebalance(Node* p, Node* c) {
    // do nothing
}

inline Node* lockParent(Node* x) {
    while (true) {
        Node* p;
        p = x->parent;
        __lock__(p->treeLock);
        if (x->parent == p && !p->mark) return p;
        __unlock__(p->treeLock);
    }
}

inline void updateChild(Node* p, Node* o, Node* n) {
    if (p->left == o) p->left = n;
    else p->right = n;
    if (n != NULL) n->parent = p;
}

inline Node* prepareTreeInsertion(Node* x, Node* z, Node* c) {
    Node* curr;
    if (c == x || c == z) curr = c;
    else curr = x;
    while (true) {
        __lock__(curr->treeLock);
        if (curr == x) {
            if (curr->right == NULL) return curr;
            __unlock__(curr->treeLock);
            curr = z;
        } else {
            if (curr->left == NULL) return curr;
            __unlock__(curr->treeLock);
            curr = x;
        }
    }
}

inline void performTreeInsertion(Node* y, Node* p) {
    if (p->key < y->key) p->right = y;
    else p->left = y;
    rebalanceIns(p);
}


inline bool prepareTreeDeletion(Node* n) {
    while (true) {
        Node* np;
        __lock__(n->treeLock);
        np = lockParent(n);
        if (n->right == NULL || n->left == NULL) {
            Node* c;
            if (n->right != NULL) {
                c = n->right;
                __lock__(c->treeLock);
                return false;
            } else if (n->left != NULL) {
                c = n->left;
                __lock__(c->treeLock);
                return false;
            }
        } else {
            Node* s, p, nr;
            bool restart;
            restart = false;
            s = n->succ;
            if (s->parent != n) {
                p = s->parent;
                __lock__(p->treeLock);
                if (p != s->parent || p->mark) restart = true;
            }
            if (!restart) {
                __lock__(s->treeLock);
                if (s->right != NULL) {
                    Node* cc;
                    cc = s->right;
                    __lock__(cc->treeLock);
                }
                return true;
            }
            __unlock__(p->treeLock);
            __unlock__(np->treeLock);
            __unlock__(n->treeLock);
        }
    }
}

inline void performTreeDeletion(Node* n, bool flag) {
    Node* s, c, p;
    if (!flag) {
        if (n->right == NULL) c = n->left;
        else c = n->right;
        p = n->parent;
        updateChild(p, n, c);
    } else {
        Node* nl, nr, np;
        s = n->succ;
        c = s->right;
        p = s->parent;
        updateChild(p, s, c);
        nl = n->left;
        nr = n->right;
        s->left = nl;
        s->right = nr;
        nl->parent = s; // plankton does not idenfiy nl to be non-NULL
        if (nr != NULL) nr->parent = s;
        np = n->parent;
        updateChild(np, n, s);
        if (p == n) p = s;
        else __unlock__(s->treeLock);
        __unlock__(np->treeLock);
    }
    __unlock__(n->treeLock);
    rebalance(p, c);
}