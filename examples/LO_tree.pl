#name "Logical Ordering Tree (most general tree overlay)"


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


bool insert(data_t k) {
    Node* x, y, z, p;
    <x, z> = locate(k);
    
    assertFlow(k, z);
    if (z->key == k) {
        __unlock__(x->listLock);
        return false;
    }

    p = prepareTreeInsertion(x, z);
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


bool remove(data_t k) {
    Node* x, y, z;
    <x, y> = locate(k);

    assertFlow(k, y);
    if (y->key != k) {
        __unlock__(x->listLock);
        return false;
    }

    __lock__(y->listLock);
    prepareTreeDeletion(y);
    z = y->succ;
    y->mark = true;
    z->pred = x;
    x->succ = z; // logical deletion
    __unlock__(y->listLock);
    __unlock__(x->listLock);
    performTreeDeletion(y);
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


inline <Node*, Node*> locate(data_t k) {
    while (true) {
        Node* x, y, z;
        y = traverse(k);
        if (y->key < k) x = y;
        else x = y->pred;
        @join; // noop, makes proof faster
        __lock__(x->listLock);
        z = x->succ;
        if (x->key < k && k <= z->key && !x->mark) {
            return <x, z>;
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
    // structure
 && x->succ == NULL ==> x == Max
 && x->pred == NULL ==> x == Min
 && x == Max ==> x->succ == NULL
 && x == Min ==> x->pred == NULL
}

def @invariant[pairwise](Node* x, Node* y) {
    x->succ == y ==> x->key < y->key
 && y->pred == x ==> x->key < y->key
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
    Max->parent = NULL;
    Max->pred = Min;
    Max->succ = NULL;
    Max->mark = false;
}


//*******************************************************//
//********************* TREE STUBS **********************//
//*******************************************************//

inline Node* prepareTreeInsertion(Node* x, Node* z) {
    @stub Node::left;
    @stub Node::right;
    @stub Node::parent;
    @stub Node::treeLock;
    choose{ return x; }{ return z; }
}

inline void performTreeInsertion(Node* y, Node* p) {
    @stub Node::left;
    @stub Node::right;
    @stub Node::parent;
    @stub Node::treeLock;
}


inline void prepareTreeDeletion(Node* y) {
    @stub Node::left;
    @stub Node::right;
    @stub Node::parent;
    @stub Node::treeLock;
}

inline void performTreeDeletion(Node* y) {
    @stub Node::left;
    @stub Node::right;
    @stub Node::parent;
    @stub Node::treeLock;
}