#name "Locked Set without unlocking"


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

    curr = Head;
    __lock__(curr->lock);
    do {
        pred = curr;
        curr = pred->next;
        __lock__(curr->lock);
        k = curr->val;
    } while (k < key);

    return <pred, curr, k>;
}


bool contains(data_t key) {
	Node* pred, curr;
	data_t k;

	<pred, curr, k> = locate(key);
	return k == key;
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
        assert(pred->next == curr);
        pred->next = entry;
        return true;
    }
}


bool remove(data_t key) {
    Node* pred, curr;
    data_t k;

	<pred, curr, k> = locate(key);

    if (k > key) {
        return false;

    } else {
        Node* next;

        next = curr->next;
        __lock__(next->lock); // needed for interference precision
        assert(pred->next == curr);
        assert(curr->next == next);
        pred->next = next;
        return true;
    }
}
