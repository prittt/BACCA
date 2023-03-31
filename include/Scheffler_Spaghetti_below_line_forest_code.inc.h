bl_tree_0: if ((c+=1) >= w - 1) goto bl_break_0_0;
		if (CONDITION_B) {
			if (CONDITION_C) {
				ACTION_1
				goto bl_tree_2;
			}
			else {
				ACTION_4
				goto bl_tree_1;
			}
		}
		else {
			ACTION_1
			goto bl_tree_0;
		}
bl_tree_1: if ((c+=1) >= w - 1) goto bl_break_0_1;
		ACTION_1
		goto bl_tree_0;
bl_tree_2: if ((c+=1) >= w - 1) goto bl_break_0_2;
		if (CONDITION_C) {
			ACTION_24
			goto bl_tree_2;
		}
		else {
			ACTION_25
			goto bl_tree_1;
		}
bl_break_0_0:
		if (CONDITION_B) {
			ACTION_4
		}
		else {
			ACTION_1
		}
		goto bl_;
bl_break_0_1:
		ACTION_1
		goto bl_;
bl_break_0_2:
		ACTION_25
		goto bl_;
bl_:;
