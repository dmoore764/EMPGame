dds_message *GetMessage(dds_message_queue *queue)
{
	dds_message *result = NULL;

	if (queue->oldestMsg != queue->newestMsgPlusOne)
	{
		result = &queue->messages[queue->oldestMsg];
		queue->oldestMsg = WrapToRange(0, queue->oldestMsg + 1, ArrayCount(queue->messages));
	}

	return result;
}

void AddMessage(dds_message_queue *queue, dds_message message)
{
	bool obtainedNewPosition = false;
	int newPtrPos;
	while (!obtainedNewPosition)
	{
		int expected = queue->newestMsgPlusOne;
		newPtrPos =  WrapToRange(0, expected+1, ArrayCount(queue->messages));
		obtainedNewPosition = __sync_bool_compare_and_swap(&queue->newestMsgPlusOne, expected, newPtrPos);
		//obtainedNewPosition = OSAtomicCompareAndSwapInt(queue->newestMsgPlusOne,, &queue->newestMsgPlusOne);
	}
	int msgPos = WrapToRange(0, newPtrPos-1, ArrayCount(queue->messages));

	queue->messages[msgPos] = message;
}
