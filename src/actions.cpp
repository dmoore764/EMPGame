action *AddStartingAction(action prototype)
{
	for (int i = 0; i < ArrayCount(Actions); i++)
	{
		if (!(Actions[i].flags & ACTION_IN_USE))
		{
			Actions[i] = prototype;
			Actions[i].flags |= ACTION_IN_USE;
			Actions[i].flags |= ACTION_READY_TO_PROCESS;
			Actions[i].next = NULL;
			return &Actions[i];
		}
	}
	return NULL;
}

void AddSubsequentAction(action **a, action prototype)
{
	for (int i = 0; i < ArrayCount(Actions); i++)
	{
		if (!(Actions[i].flags & ACTION_IN_USE))
		{
			Actions[i] = prototype;
			Actions[i].flags |= ACTION_IN_USE;
			Actions[i].next = NULL;
			(*a)->next = &Actions[i];
			(*a) = (*a)->next;
			return;
		}
	}
}

void ProcessAction(action *a, float deltaT)
{
	local_persistent cam_update *currentCamUpdate;
	switch (a->type)
	{
		case ACTION_Wait:
		{
			a->wait.timeToWait -= deltaT;
			if (a->wait.timeToWait <= 0)
			{
				a->flags |= ACTION_COMPLETE;
			}
		} break;

		case ACTION_Show_dialogue_box:
		{
			CreateDialogueBox();
			a->flags |= ACTION_COMPLETE;
		} break;

		case ACTION_Add_dialogue_string:
		{
			if (!a->add_dialogue_string.init)
			{
				DBAddLine(a->add_dialogue_string.string);
				a->add_dialogue_string.init = true;
			}
			if (DialogueBox.drawTo == DialogueBox.numCharacters)
			{
				a->flags |= ACTION_COMPLETE;
			}
		} break;

		case ACTION_Wait_for_dialogue_advance:
		{
			if (KeyPresses[KEY_Action1])
			{
				KeyPresses[KEY_Action1] = false;
				a->flags |= ACTION_COMPLETE;
			}
		} break;

		case ACTION_Remove_dialogue_box:
		{
			RemoveDialogueBox();
			a->flags |= ACTION_COMPLETE;
		} break;

		case ACTION_Remove_character_control:
		{
			int playerID = GetFirstOfType(OBJ_PLAYER);
			if (playerID >= 0)
			{
				auto flag = OTH(playerID, custom_flags);
				flag->bits |= CONTROL_LOCKED;
			}
			a->flags |= ACTION_COMPLETE;
		} break;

		case ACTION_Return_character_control:
		{
			int playerID = GetFirstOfType(OBJ_PLAYER);
			if (playerID >= 0)
			{
				auto flag = OTH(playerID, custom_flags);
				flag->bits &= ~CONTROL_LOCKED;
			}
			a->flags |= ACTION_COMPLETE;
		} break;

		case ACTION_Freeze_camera:
		{
			currentCamUpdate = CameraUpdateFunction;
			CameraUpdateFunction = CameraFreeze;
			a->flags |= ACTION_COMPLETE;
		} break;

		case ACTION_Unfreeze_camera:
		{
			CameraUpdateFunction = currentCamUpdate;
			a->flags |= ACTION_COMPLETE;
		} break;

		case ACTION_Change_update_function:
		{
			auto update = OTH(a->change_update_function.objectID, update);
			update->update = a->change_update_function.function;
			a->flags |= ACTION_COMPLETE;
		} break;

		case ACTION_Set_animation:
		{
			auto anim = OTH(a->set_animation.objectID, anim);
			anim->name = a->set_animation.animation;
			anim->playing = true;
			anim->currentFrame = 0;
			anim->frameTime = 0;
			anim->speedFactor = 1;
			a->flags |= ACTION_COMPLETE;
		} break;

		case ACTION_Set_speed:
		{
			auto phy = OTH(a->set_speed.objectID, physics);
			phy->vel = {a->set_speed.setX ? a->set_speed.speed.x : phy->vel.x,
						a->set_speed.setY ? a->set_speed.speed.y : phy->vel.y};

			if (a->set_speed.usesFinishX)
			{
				auto tx = OTH(a->set_speed.objectID, transform);
				if ((a->set_speed.greaterThan && (tx->pos.x > a->set_speed.finishX)) || (!a->set_speed.greaterThan && (tx->pos.x <= a->set_speed.finishX)))
				{
					a->flags |= ACTION_COMPLETE;
					if (a->set_speed.stopOnFinish)
						phy->vel = {0,0};
					if (a->set_speed.setToPosOnFinish)
						tx->pos.x = a->set_speed.finishX;
				}
			}
			else if (a->set_speed.usesFinishY)
			{
				auto tx = OTH(a->set_speed.objectID, transform);
				if ((a->set_speed.greaterThan && (tx->pos.y > a->set_speed.finishY)) || (!a->set_speed.greaterThan && (tx->pos.y <= a->set_speed.finishY)))
				{
					a->flags |= ACTION_COMPLETE;
					if (a->set_speed.stopOnFinish)
						phy->vel = {0,0};
					if (a->set_speed.setToPosOnFinish)
						tx->pos.y = a->set_speed.finishY;
				}
			}
			else
			{
				a->flags |= ACTION_COMPLETE;
			}
		} break;

		case ACTION_Pause_game:
		{
			SendingGameUpdateEvents = false;
			a->flags |= ACTION_COMPLETE;
		} break;

		case ACTION_Resume_game:
		{
			SendingGameUpdateEvents = true;
			a->flags |= ACTION_COMPLETE;
		} break;

	}

	if (a->flags & ACTION_COMPLETE)
	{
		a->flags &= ~ACTION_IN_USE;
	}

	if (a->next && ((a->flags & ACTION_COMPLETE) || (a->flags & ACTION_PROCESS_NEXT_IMMEDIATELY)))
	{
		a->next->flags |= ACTION_PROCESSING;
		ProcessAction(a->next, deltaT);
		a->next = NULL;
	}
}

void ProcessActions(float deltaT)
{
	for (int i = 0; i < ArrayCount(Actions); i++)
	{
		action *a = &Actions[i];
		if ((a->flags & ACTION_IN_USE) && ((a->flags & ACTION_READY_TO_PROCESS) || (a->flags & ACTION_PROCESSING)))
		{
			a->flags |= ACTION_PROCESSING;
			ProcessAction(a, deltaT);
		}
	}
}

void ClearActions()
{
	for (int i = 0; i < ArrayCount(Actions); i++)
	{
		action *a = &Actions[i];
		a->flags &= ~ACTION_IN_USE;
	}
}

void InitGlobalActions()
{
	FREEZE_CAMERA.type = ACTION_Freeze_camera;
	UNFREEZE_CAMERA.type = ACTION_Unfreeze_camera;
	WAIT_FOR_KEY.type = ACTION_Wait_for_dialogue_advance;
	REMOVE_DIALOGUE_BOX.type = ACTION_Remove_dialogue_box;
	REMOVE_CHARACTER_CONTROL.type = ACTION_Remove_character_control;
	RETURN_CHARACTER_CONTROL.type = ACTION_Return_character_control;
}
