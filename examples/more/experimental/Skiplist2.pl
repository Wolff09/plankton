#name "Lazy Skip List for Height 2"


struct Node {
    thread_t lock;
	data_t val;
	Node* next1;
	Node* next2;
	bool marked;
	bool linked;
	bool topLevel;
}

Node* Head;
Node* Tail;


def @contains(Node* node, data_t key) {
    node->val == key
}

def @outflow[next1](Node* node, data_t key) {
    node->val < key
}

def @outflow[next2](Node* node, data_t key) {
    false
}

def @invariant[local](Node* x) {
    x->_flow == 0
}

def @invariant[shared](Node* x) {
    Head != NULL
 && Head->next1 != NULL
 && Head->next2 != NULL
 && Head->val == MIN
 && Head->marked == false
 && Head->linked == true
 && Head->topLevel == true
 && Head->_flow != 0
 && [MIN, MAX] in Head->_flow
 && x->val == MIN ==> x == Head

 && Tail != NULL
 && Tail->next1 == NULL
 && Tail->next2 == NULL
 && Tail->val == MAX
 && Tail->marked == false
 && Tail->linked == true
 && Tail->topLevel == true
 && Tail->_flow != 0
 && x->val == MAX ==> x == Tail

 && x->_flow != 0 ==> [x->val, MAX] in x->_flow
 && x->val != MAX ==> x->next1 != NULL
 && (x->val != MAX && x->topLevel) ==> x->next2 != NULL
 && x->next2 != NULL ==> x->next1 != NULL
 && (x->_flow != 0 && x->next1 != NULL) ==> x->val != MAX
 && x->marked == false ==> x->_flow != 0
}


inline <Node*, Node*, Node*, Node*, data_t, data_t> locate(data_t key) {
	Node* pred1, pred2, curr1, curr2, node;
	data_t k1, k2;

	curr2 = Head;
	do {
		pred2 = curr2;
		curr2 = pred2->next2;
		assume(curr2 != NULL); // HINT!
		k2 = curr2->val;
	} while (key > k2);

	curr1 = Head;
	do {
		pred1 = curr1;
		curr1 = pred1->next1;
		k1 = curr1->val;
	} while (key > k1);

	return <pred1, pred2, curr1, curr2, k1, k2>;
}


void __init__() {
	Tail = malloc;
	Tail->next1 = NULL;
	Tail->next2 = NULL;
	Tail->val = MAX;
	Tail->marked = false;
	Tail->linked = true;
	Head = malloc;
	Tail->next1 = Tail;
	Tail->next2 = Tail;
	Tail->val = MIN;
	Tail->marked = false;
	Tail->linked = true;
}


// bool contains(data_t key) {
// 	Node* pred1, pred2, succ1, succ2, entry;
// 	data_t k1, k2;
//
// 	<pred1, pred2, succ1, succ2, k1, k2> = locate(key);
// 	if (k2 == key && succ2->linked && !succ2->marked) return true;
// 	else if (k1 == key && succ1->linked && !succ1->marked) return true;
// 	else return false;
// }


bool add(data_t key) {
	Node* pred1, pred2, succ1, succ2, entry;
	data_t k1, k2;

	entry = malloc;
	entry->val = key;
	entry->marked = false;
	entry->linked = false;

	while (true) {
		<pred1, pred2, succ1, succ2, k1, k2> = locate(key);

		// found at level 2
		if (key == k2) {
			if (!succ2->marked) {
				while (!succ2->linked) {}
				return false;
			}
		}

		// found at level 1
		else if (key == k1) {
			if (!succ1->marked) {
				while (!succ1->linked) {}
				return false;
			}
		}

		// not found ~> inserting
		else {
			choose {
				// topLevel = false
				__lock__(pred1->lock);
				if (!pred1->marked && !succ1->marked && pred1->next1 == succ1) {
					entry->topLevel = false;
					entry->next1 = succ1;
					entry->next2 = NULL;
					pred1->next1 = entry;
					entry->linked = true;
					__unlock__(pred1->lock);
					return true;
				}

			}{
				// topLevel = true
				__lock__(pred1->lock);
				if (!pred1->marked && !succ1->marked && pred1->next1 == succ1) {
					if (pred1 != pred2) __lock__(pred2->lock);
					if (!pred2->marked && !succ2->marked && pred2->next2 == succ2) {
						entry->topLevel = true;
						entry->next1 = succ1;
						entry->next2 = succ2;
						pred1->next1 = entry;
						pred2->next2 = entry;
						entry->linked = true;
						__unlock__(pred1->lock);
						if (pred1 != pred2) __unlock__(pred2->lock);
						return true;
					}
				}
			}
		}
	}
}


// bool remove(data_t key) {
// 	Node* pred1, pred2, succ1, succ2, victim, next1, next2;
// 	data_t k1, k2;
// 	bool isMarked, found, foundTopLevel, topLevel, valid;
//
// 	isMarked = false;
// 	topLevel = false;
// 	victim = NULL;
//
// 	while (true) {
// 		found = false;
// 		<pred1, pred2, succ1, succ2, k1, k2> = locate(key);
// 		if (k2 == key) { victim = succ2; found = true; foundTopLevel = true; }
// 		else if (k1 == key) { victim = succ1; found = true; foundTopLevel = false; }
//
//         valid = found && victim->linked && victim->topLevel == foundTopLevel && !victim->marked;
// 		if (isMarked || valid) {
// 			// logical deletion
// 			if (!isMarked) {
// 				topLevel = victim->topLevel;
// 				__lock__(victim->lock);
// 				if (victim->marked) {
// 					__unlock__(victim->lock);
// 					return false;
// 				} else {
// 					victim->marked = true;
// 					isMarked = true;
// 				}
// 			}
//
// 			// physical deletion for 1 level
// 			if (topLevel) {
// 				__lock__(pred1->lock);
// 				if (!pred1->marked && pred1->next1 == victim) {
// 				    next1 = victim->next1;
// 					pred1->next1 = next1;
// 					__unlock__(victim->lock);
// 					__unlock__(pred1->lock);
// 					return true;
// 				}
//
// 			// physical deletion for 2 levels
// 			} else {
// 				__lock__(pred1->lock);
// 				if (!pred1->marked && pred1->next1 == victim) {
// 					if (pred1 != pred2) __lock__(pred2->lock);
// 					if (!pred2->marked && pred2->next2 == victim) {
// 					    next1 = victim->next1;
// 					    next2 = victim->next2;
// 						pred1->next1 = next1;
// 						pred2->next2 = next2;
// 						__unlock__(victim->lock);
// 						__unlock__(pred1->lock);
// 						if (pred1 != pred2) __unlock__(pred2->lock);
// 						return true;
// 					}
// 				}
// 			}
//
// 		} else return false;
// 	}
// }
