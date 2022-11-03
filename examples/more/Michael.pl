#name "Michael Set"


struct Node {
	data_t val;
	bool marked;
	Node* next;
}

Node* Head;
Node* Tail;


def @contains(Node* node, data_t key) {
    !node->marked && node->val == key
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
 && !Head->marked
 && Head->_flow != 0
 && [MIN, MAX] in Head->_flow
 && x->val == MIN ==> x == Head

 && Tail != NULL
 && Tail->next == NULL
 && Tail->val == MAX
 && !Tail->marked
 && Tail->_flow != 0
 && x->val == MAX ==> x == Tail
 && x->next == NULL ==> x == Tail
 && x->next != NULL ==> x->val != MAX

 && !x->marked ==> [x->val, MAX] in x->_flow
 && x->_flow == 0 ==> x->marked
 && x->_flow != 0 ==> [x->val, MAX] in x->_flow
}


void __init__() {
	Tail = malloc;
	Tail->next = NULL;
	Tail->marked = false;
	Tail->val = MAX;
	Head = malloc;
	Head->next = Tail;
	Head->marked = false;
	Head->val = MIN;
}


inline <Node*, Node*, data_t> locate(data_t key) {
	Node* curr, pred, next;
	data_t k;

	curr = Head;
	do {
		pred = curr;
		curr = pred->next;
        if (pred->marked == false && pred->next == curr) {
			k = curr->val;
			if (curr->marked == true) {
			    next = curr->next;
			    CAS(<pred->marked, pred->next>, <false, curr>, <false, next>);
			    // retry
			    curr = Head;
			    k = MIN;
			}
		} else {
            // retry
			curr = Head;
			k = MIN;
		}
	} while (k < key);
    return <pred, curr, k>;
}


bool contains(data_t key) {
	Node* curr, pred;
	data_t k;

	<pred, curr, k> = locate(key);
	return k == key;
}

bool add(data_t key) {
	Node* entry;
	entry = malloc;
	entry->val = key;
	entry->marked = false;

	while (true) {
        Node* curr, pred;
        data_t k;
		<pred, curr, k> = locate(key);

		if (k == key) {
            return false;

		} else {
			entry->next = curr;
            if (CAS(<pred->marked, pred->next>, <false, curr>, <false, entry>)) {
				return true;
			}
		}
	}
}

bool remove(data_t key) {
	while (true) {
        Node* curr, pred, next;
        data_t k;
		<pred, curr, k> = locate(key);

		if (k > key) {
			return false;

		} else {
            next = curr->next;
			if (CAS(<curr->marked, curr->next>, <false, next>, <true, next>)) {
                CAS(<pred->marked, pred->next>, <false, curr>, <false, next>);
                // <pred, curr, k> = locate(key);
                return true;
			}
		}
	}
}
