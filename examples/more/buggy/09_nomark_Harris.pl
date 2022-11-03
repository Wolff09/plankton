#name "BUGGY Harris Set"


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
	while (true) {
		Node* left, lnext, right, rnext;
		bool rmark;
		data_t k;

		// traverse
		right = Head;
		<rmark, rnext> = <Head->marked, Head->next>;
		do {
		    assume(rmark || k < key); // repeated loop condition for join precision
			if (!rmark) {
				left = right;
				lnext = rnext;
			}
			right = rnext;
			k = right->val;
			if (right == Tail) break;
			<rmark, rnext> = <right->marked, right->next>;
		} while (rmark || k < key);

		// left and right are successors
		if (lnext == right) {
			if (right == Tail || !right->marked) {
				return <left, right, k>;
			}
		}

		// unlink marked nodes between left and right (potentially unboundedly many)
		if (CAS(<left->marked, left->next>, <false, lnext>, <false, right>)) {
			if (right == Tail || !right->marked) {
				return <left, right, k>;
			}
		}
	}
}

bool contains(data_t key) {
	Node* left, right;
	data_t k;

	<left, right, k> = locate(key);
	return k == key;
}

bool add(data_t key) {
	Node* entry;

	entry = malloc;
	entry->val = key;
	entry->marked = false;

	while (true) {
		Node* left, right;
		data_t k;
		<left, right, k> = locate(key);

		if (k == key) {
            return false;

		} else {
			entry->next = right;
            if (CAS(<left->marked, left->next>, <false, right>, <false, entry>)) {
				return true;
			}
		}
	}
}

bool remove(data_t key) {
	while (true) {
		Node* left, right;
		data_t k;
		<left, right, k> = locate(key);

		if (k > key) {
			return false;

		} else {
			Node* next;
            next = right->next;
			// if (CAS(<right->marked, right->next>, <false, next>, <true, next>)) { // buggy: no logical deletion
            if (CAS(<left->marked, left->next>, <false, right>, <false, next>)) {
                // <left, right, k> = locate(key);
                return true;
			}
		}
	}
}
