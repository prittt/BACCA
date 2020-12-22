// Copyright (c) 2020, the BACCA contributors, as
// shown by the AUTHORS file. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.
//
// The following code has been generated with GRAPHGEN:
// https://github.com/prittt/GRAPHGEN

sl_tree_0: if ((c+=1) >= w - 1) goto sl_break_0_0;
		if (CONDITION_X) {
			if (CONDITION_E) {
				ACTION_3
				goto sl_tree_2;
			}
			else {
				ACTION_5
				goto sl_tree_1;
			}
		}
		else {
			ACTION_1
			goto sl_tree_0;
		}
sl_tree_1: if ((c+=1) >= w - 1) goto sl_break_0_1;
		ACTION_1
		goto sl_tree_0;
sl_tree_2: if ((c+=1) >= w - 1) goto sl_break_0_2;
		if (CONDITION_E) {
			ACTION_61
			goto sl_tree_2;
		}
		else {
			ACTION_62
			goto sl_tree_1;
		}
sl_break_0_0:
		if (CONDITION_X) {
			ACTION_5
		}
		else {
			ACTION_1
		}
		goto sl_;
sl_break_0_1:
		ACTION_1
		goto sl_;
sl_break_0_2:
		ACTION_62
		goto sl_;
sl_:;
