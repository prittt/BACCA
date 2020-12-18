// Copyright (c) 2020, the BACCA contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

fl_tree_0: if ((c+=1) >= w - 1) goto fl_break_0_0;
		if (CONDITION_X) {
			if (CONDITION_G) {
				ACTION_3
				goto fl_tree_4;
			}
			else {
				if (CONDITION_E) {
					ACTION_3
					goto fl_tree_3;
				}
				else {
					NODE_16:
					if (CONDITION_H) {
						ACTION_3
						goto fl_tree_2;
					}
					else {
						ACTION_5
						goto fl_tree_2;
					}
				}
			}
		}
		else {
			ACTION_1
			goto fl_tree_1;
		}
fl_tree_1: if ((c+=1) >= w - 1) goto fl_break_0_1;
		if (CONDITION_X) {
			if (CONDITION_G) {
				ACTION_3
				goto fl_tree_4;
			}
			else {
				if (CONDITION_E) {
					if (CONDITION_F) {
						ACTION_4
						goto fl_tree_3;
					}
					else {
						ACTION_3
						goto fl_tree_3;
					}
				}
				else {
					if (CONDITION_F) {
						if (CONDITION_H) {
							ACTION_4
							goto fl_tree_2;
						}
						else {
							ACTION_3
							goto fl_tree_2;
						}
					}
					else {
						goto NODE_16;
					}
				}
			}
		}
		else {
			ACTION_1
			goto fl_tree_1;
		}
fl_tree_2: if ((c+=1) >= w - 1) goto fl_break_0_2;
		ACTION_1
		goto fl_tree_1;
fl_tree_3: if ((c+=1) >= w - 1) goto fl_break_0_3;
		if (CONDITION_G) {
			ACTION_45
			goto fl_tree_4;
		}
		else {
			if (CONDITION_E) {
				ACTION_61
				goto fl_tree_3;
			}
			else {
				if (CONDITION_H) {
					ACTION_61
					goto fl_tree_2;
				}
				else {
					ACTION_62
					goto fl_tree_2;
				}
			}
		}
fl_tree_4: if ((c+=1) >= w - 1) goto fl_break_0_4;
		if (CONDITION_X) {
			if (CONDITION_G) {
				ACTION_45
				goto fl_tree_4;
			}
			else {
				if (CONDITION_E) {
					ACTION_46
					goto fl_tree_3;
				}
				else {
					if (CONDITION_H) {
						ACTION_46
						goto fl_tree_2;
					}
					else {
						ACTION_45
						goto fl_tree_2;
					}
				}
			}
		}
		else {
			ACTION_1
			goto fl_tree_1;
		}
fl_break_0_0:
		if (CONDITION_X) {
			if (CONDITION_G) {
				ACTION_3
			}
			else {
				ACTION_5
			}
		}
		else {
			ACTION_1
		}
		goto fl_;
fl_break_0_1:
		if (CONDITION_X) {
			if (CONDITION_G) {
				ACTION_3
			}
			else {
				if (CONDITION_F) {
					ACTION_3
				}
				else {
					ACTION_5
				}
			}
		}
		else {
			ACTION_1
		}
		goto fl_;
fl_break_0_2:
		ACTION_1
		goto fl_;
fl_break_0_3:
		if (CONDITION_G) {
			ACTION_45
		}
		else {
			ACTION_62
		}
		goto fl_;
fl_break_0_4:
		if (CONDITION_X) {
			ACTION_45
		}
		else {
			ACTION_1
		}
		goto fl_;
fl_:;
