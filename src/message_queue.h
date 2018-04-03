enum dds_message_type
{
	DDS_TEXTURE_UPDATED,
	DDS_SHADER_UPDATED,
	DDS_IMAGE_FILES_LOADED,
};

struct dds_message
{
	dds_message_type type;
	void *data;
};

struct dds_message_queue
{
	dds_message messages[128];
	int oldestMsg;
	int newestMsgPlusOne;
};

dds_message *GetMessage(dds_message_queue *queue);
void AddMessage(dds_message_queue *queue, dds_message message);
