#name "BUGGY Vechev&Yahav CAS Set"


struct Node {
	data_t val;
	bool marked;
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
	Node* pred, curr;
	data_t k;

	curr = Head;
	do {
		pred = curr;
		k = curr->val; // buggy: wrong key
		curr = pred->next;
        // k = curr->val; // correct
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
	entry->marked = false;

	while (true) {
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
	Node* pred, curr, next;
	data_t k;

	while (true) {
		<pred, curr, k> = locate(key);

		if (k > key) {
			return false;

		} else {
            // TODO: support any curr->marked, not just unmarked
            next = curr->next;
			if (CAS(<curr->marked, curr->next>, <false, next>, <true, next>)) {
                if (CAS(<pred->marked, pred->next>, <false, curr>, <false, next>)) {
                    return true;
                }
			}
		}
	}
}
