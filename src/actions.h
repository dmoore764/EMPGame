enum action_flags
{
	ACTION_IN_USE						= 0x1,
	ACTION_READY_TO_PROCESS				= 0x2,
	ACTION_PROCESSING					= 0x4,
	ACTION_PROCESS_NEXT_IMMEDIATELY		= 0x8,
	ACTION_COMPLETE						= 0x10,
};

enum action_type
{
	ACTION_Wait,
	ACTION_Show_dialogue_box,
	ACTION_Add_dialogue_string,
	ACTION_Wait_for_dialogue_advance,
	ACTION_Remove_dialogue_box,

	ACTION_Remove_character_control,
	ACTION_Return_character_control,

	ACTION_Freeze_camera,
	ACTION_Unfreeze_camera,

	ACTION_Change_update_function,

	ACTION_Set_animation,
	ACTION_Set_speed,

	ACTION_Pause_game,
	ACTION_Resume_game,
};

struct action
{
	uint32_t flags;
	action_type type;

	union
	{
		struct
		{
			float timeToWait;
		} wait;

		struct
		{
			bool init;
			char *string;
		} add_dialogue_string;

		struct
		{
			int objectID;
			update_function *function;
		} change_update_function;

		struct
		{
			int objectID;
			char *animation;
		} set_animation;

		struct
		{
			int objectID;
			bool setX;
			bool setY;
			ivec2 speed;
			bool stopOnFinish;
			bool setToPosOnFinish;
			bool greaterThan;
			bool usesFinishX;
			uint32_t finishX;
			bool usesFinishY;
			uint32_t finishY;
		} set_speed;
	};

	action *next;
};

global_variable action Actions[128];

action *AddStartingAction(action prototype);
void AddSubsequentAction(action **a, action prototype);
void ProcessAction(action *a, float deltaT);
void ProcessActions(float deltaT);
void InitGlobalActions();


global_variable action FREEZE_CAMERA;
global_variable action UNFREEZE_CAMERA;
global_variable action WAIT_FOR_KEY;
global_variable action REMOVE_DIALOGUE_BOX;
global_variable action REMOVE_CHARACTER_CONTROL;
global_variable action RETURN_CHARACTER_CONTROL;
