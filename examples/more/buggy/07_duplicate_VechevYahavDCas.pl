#name "BUGGY Vechev&Yahav DCAS Set"


struct Node {
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

 && x->next != NULL ==> (x->_flow != 0 && [x->val, MAX] in x->_flow)
 && x->_flow != 0 ==> [x->val, MAX] in x->_flow
 && (x->_flow != 0 && x->next != NULL) ==> x->val != MAX
 && (x->_flow != 0 && x->val != MAX) ==> x->next != NULL
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

		if (curr == pred->next && curr != NULL) {
			k = curr->val;
		} else {
			curr = Head;
			k = MIN;
		}
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
	Node* entry;

	entry = malloc;
	entry->val = key;

	while (true) {
		Node* pred, curr;
		data_t k;

		<pred, curr, k> = locate(key);

        // buggy: ignoring already present key
		// if (k == key) {
        //     return false;
        //
		// } else {
			entry->next = curr;
			if (CAS(pred->next, curr, entry)) {
				return true;
			}
		}
	}
}


bool remove(data_t key) {
	while (true) {
		Node* pred, curr;
		data_t k;

		<pred, curr, k> = locate(key);

		if (k > key) {
			return false;

		} else {
			Node* next;

			next = curr->next;
			if (CAS(<pred->next, curr->next>, <curr, next>, <next, NULL>)) {
				return true;
			}
		}
	}
}
