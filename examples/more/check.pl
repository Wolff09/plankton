#name "Installation Check"

/* NOTE
 * this is a mock implementation for testing the plankton installation
 */

struct Node {
    thread_t lock;
    data_t val;
    Node* next;
}

Node* Head;
Node* Tail;


def @contains(Node* node, data_t key) {
    node->val == key
}

def @outflow[next](Node* node, data_t key) {
    node->val < key
}

def @invariant[local](Node* x) {
    x->_flow == 0
}

def @invariant[shared](Node* x) {
    Head != NULL
 && Head->next != NULL
 && Head->val == MIN
 && Head->_flow != 0
 && [MIN, MAX] in Head->_flow
 && x->val == MIN ==> x == Head

 && Tail != NULL
 && Tail->next == NULL
 && Tail->val == MAX
 && Tail->_flow != 0
 && x->val == MAX ==> x == Tail

 && x->_flow != 0 ==> [x->val, MAX] in x->_flow
 && (x->_flow != 0 && x->val != MAX) ==> x->next != NULL
 && (x->_flow != 0 && x->next != NULL) ==> x->val != MAX
}


void __init__() {
    Tail = malloc;
    Tail->next = NULL;
    Tail->val = MAX;
    Head = malloc;
    Head->next = Tail;
    Head->val = MIN;
}


inline <Node*, Node*, data_t> locate(data_t key) {
    Node* pred, curr;
    data_t k;

    pred = Head;
    __lock__(pred->lock);
    curr = pred->next;
    __lock__(curr->lock);
    k = curr->val;
    while (k < key) {
        pred = curr;
        curr = pred->next;
        __lock__(curr->lock);
        k = curr->val;
    }

    return <pred, curr, k>;
}


bool add(data_t key) {
    Node* entry, pred, curr;
    data_t k;

    entry = malloc;
    entry->val = key;

    <pred, curr, k> = locate(key);

    if (k == key) {
        return false;

    } else {
        entry->next = curr;
        pred->next = entry;
        return true;
    }
}
