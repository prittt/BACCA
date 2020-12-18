// Copyright (c) 2020, the BACCA contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

ll_tree_0: if ((c+=1) >= w - 1) goto ll_break_0_0;
		if (CONDITION_X) {
			if (CONDITION_B) {
				NODE_9:
				if (CONDITION_E) {
					ACTION_15
					goto ll_tree_5;
				}
				else {
					if (CONDITION_C) {
						ACTION_19
						goto ll_tree_2;
					}
					else {
						ACTION_22
						goto ll_tree_2;
					}
				}
			}
			else {
				NODE_10:
				if (CONDITION_C) {
					if (CONDITION_E) {
						ACTION_8
						goto ll_tree_4;
					}
					else {
						ACTION_12
						goto ll_tree_2;
					}
				}
				else {
					if (CONDITION_E) {
						ACTION_3
						goto ll_tree_3;
					}
					else {
						ACTION_5
						goto ll_tree_2;
					}
				}
			}
		}
		else {
			ACTION_1
			goto ll_tree_1;
		}
ll_tree_1: if ((c+=1) >= w - 1) goto ll_break_0_1;
		if (CONDITION_X) {
			if (CONDITION_B) {
				if (CONDITION_A) {
					if (CONDITION_E) {
						ACTION_29
						goto ll_tree_5;
					}
					else {
						if (CONDITION_C) {
							ACTION_33
							goto ll_tree_2;
						}
						else {
							ACTION_36
							goto ll_tree_2;
						}
					}
				}
				else {
					goto NODE_9;
				}
			}
			else {
				if (CONDITION_A) {
					if (CONDITION_C) {
						if (CONDITION_E) {
							ACTION_40
							goto ll_tree_4;
						}
						else {
							ACTION_44
							goto ll_tree_2;
						}
					}
					else {
						if (CONDITION_E) {
							ACTION_37
							goto ll_tree_3;
						}
						else {
							ACTION_39
							goto ll_tree_2;
						}
					}
				}
				else {
					goto NODE_10;
				}
			}
		}
		else {
			ACTION_1
			goto ll_tree_1;
		}
ll_tree_2: if ((c+=1) >= w - 1) goto ll_break_0_2;
		ACTION_1
		goto ll_tree_1;
ll_tree_3: if ((c+=1) >= w - 1) goto ll_break_0_3;
		if (CONDITION_C) {
			if (CONDITION_E) {
				ACTION_63
				goto ll_tree_4;
			}
			else {
				ACTION_65
				goto ll_tree_2;
			}
		}
		else {
			if (CONDITION_E) {
				ACTION_61
				goto ll_tree_3;
			}
			else {
				ACTION_62
				goto ll_tree_2;
			}
		}
ll_tree_4: if ((c+=1) >= w - 1) goto ll_break_0_4;
		NODE_11:
		if (CONDITION_E) {
			ACTION_51
			goto ll_tree_5;
		}
		else {
			if (CONDITION_C) {
				ACTION_53
				goto ll_tree_2;
			}
			else {
				ACTION_55
				goto ll_tree_2;
			}
		}
ll_tree_5: if ((c+=1) >= w - 1) goto ll_break_0_5;
		if (CONDITION_B) {
			goto NODE_11;
		}
		else {
			if (CONDITION_C) {
				if (CONDITION_E) {
					ACTION_58
					goto ll_tree_4;
				}
				else {
					ACTION_60
					goto ll_tree_2;
				}
			}
			else {
				if (CONDITION_E) {
					ACTION_56
					goto ll_tree_3;
				}
				else {
					ACTION_57
					goto ll_tree_2;
				}
			}
		}
ll_break_0_0:
		if (CONDITION_X) {
			if (CONDITION_B) {
				ACTION_22
			}
			else {
				ACTION_5
			}
		}
		else {
			ACTION_1
		}
		goto ll_;
ll_break_0_1:
		if (CONDITION_X) {
			if (CONDITION_B) {
				if (CONDITION_A) {
					ACTION_36
				}
				else {
					ACTION_22
				}
			}
			else {
				if (CONDITION_A) {
					ACTION_39
				}
				else {
					ACTION_5
				}
			}
		}
		else {
			ACTION_1
		}
		goto ll_;
ll_break_0_2:
		ACTION_1
		goto ll_;
ll_break_0_3:
		ACTION_62
		goto ll_;
ll_break_0_4:
		ACTION_55
		goto ll_;
ll_break_0_5:
		if (CONDITION_B) {
			ACTION_55
		}
		else {
			ACTION_57
		}
		goto ll_;
ll_:;
