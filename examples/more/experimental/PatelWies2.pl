#name "Patel&Wies set"


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


bool contains(data_t key) {
    Node* right, rnext;
    data_t k;

    right = Head;
    rnext = Head->next;
    do {
        assume(k < key); // repeated loop condition for join precision
        right = rnext;
        k = right->val;
        if (right == Tail) break;
        rnext = right->next;
    } while (k < key);

	return k == key && !right->marked;
}

bool add(data_t key) {
	Node* entry;

	entry = malloc;
	entry->val = key;
	entry->marked = false;

	while (true) {
		Node* left, lnext, right;
        while (true) {
            Node* rnext;
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
                if (k == key && !rmark) return false;
                if (right == Tail) break;
                <rmark, rnext> = <right->marked, right->next>;
            } while (rmark || k < key);

            if (right == Tail || !right->marked) {
                break;
            }
        }

		if (right->val == key) {
            return false;

		} else {
			entry->next = right;
			if (CAS(<left->marked, left->next>, <false, lnext>, <false, entry>)) {
                return true;
            }
		}
	}
}

bool remove(data_t key) {
	while (true) {
		Node* left, lnext, right;
        while (true) {
            Node* rnext;
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
                if (k == key && !rmark) return false;
                if (right == Tail) break;
                <rmark, rnext> = <right->marked, right->next>;
            } while (rmark || k < key);

            if (right == Tail || !right->marked) {
                break;
            }
        }

		if (right->val > key) {
			return false;

		} else {
			Node* next;
            next = right->next;
			if (CAS(<right->marked, right->next>, <false, next>, <true, next>)) {
			    CAS(<left->marked, left->next>, <false, lnext>, <false, next>);
                // <left, right> = locate(key);
                return true;
			}
		}
	}
}
