#name "Coarse Locked Set"

/* NOTE:
 * Verification will fail for this implementation because the interference
 * does not pick up the lock on Head when modifying unrelated nodes.
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

    curr = Head;
    do {
        pred = curr;
        curr = pred->next;
        k = curr->val;
    } while (k < key);

    return <pred, curr, k>;
}


bool contains(data_t key) {
	Node* pred, curr;
	data_t k;

    __lock__(Head->lock);
	<pred, curr, k> = locate(key);
	__unlock__(Head->lock);
	return k == key;
}


bool add(data_t key) {
	Node* entry, pred, curr;
    data_t k;

	entry = malloc;
	entry->val = key;

    __lock__(Head->lock);
	<pred, curr, k> = locate(key);

    if (k == key) {
	    __unlock__(Head->lock);
        return false;

    } else {
        entry->next = curr;
        assert(pred->next == curr);
        pred->next = entry;
        __unlock__(Head->lock);
        return true;
    }
}


bool remove(data_t key) {
    Node* pred, curr;
    data_t k;

    __lock__(Head->lock);
	<pred, curr, k> = locate(key);

    if (k > key) {
        __unlock__(Head->lock);
        return false;

    } else {
        Node* next;

        next = curr->next;
        assert(pred->next == curr);
        assert(curr->next == next);
        pred->next = next;
	    __unlock__(Head->lock);
        return true;
    }
}
