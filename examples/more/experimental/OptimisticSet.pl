#name "Optimistic Set with Hint"


struct Node {
    thread_t lock;
	data_t val;
	bool mark;
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
 && (x->val != MAX) ==> x->next != NULL
 && (x->_flow != 0 && x->next != NULL) ==> x->val != MAX

 && x->mark == false ==> x->_flow != 0
 && x->mark == true ==> x->_flow == 0
 && x->_flow != 0 ==> x->mark == false
 && x->_flow == 0 ==> x->mark == true
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
	Node* pred, curr, node;
	data_t k;

    while (true) {
        curr = Head;
        do {
            pred = curr;
            curr = pred->next;
            k = curr->val;
        } while (k < key);

        __lock__(pred->lock);
        __lock__(curr->lock);

        node = Head;
        while (node->val <= pred->val) {
            if (node == pred) {
                assert(pred->mark == false);
                if (pred->next == curr) return <pred, curr, k>;
                else break;
            } else {
                node = node->next;
            }
        }
    }
}


bool contains(data_t key) {
	Node* pred, curr;
	data_t k;

	<pred, curr, k> = locate(key);
	__unlock__(pred->lock);
	__unlock__(curr->lock);
	return k == key;
}


bool add(data_t key) {
	Node* entry, pred, curr;
    data_t k;

	entry = malloc;
	entry->val = key;
	entry->mark = false;

	<pred, curr, k> = locate(key);

    if (k == key) {
        __unlock__(pred->lock);
        __unlock__(curr->lock);
        return false;

    } else {
        entry->next = curr;
        // assert(pred->next == curr);
        pred->next = entry;
        __unlock__(pred->lock);
        __unlock__(curr->lock);
        return true;
    }
}


bool remove(data_t key) {
    Node* pred, curr;
    data_t k;

	<pred, curr, k> = locate(key);

    if (k > key) {
        __unlock__(pred->lock);
        __unlock__(curr->lock);
        return false;

    } else {
        Node* next;

        next = curr->next;
        // assert(pred->next == curr);
        // assert(curr->next == next);
        // curr->mark = true;
        <pred->next, curr->mark> = <next, true>;
        __unlock__(pred->lock);
        __unlock__(curr->lock);
        return true;
    }
}
