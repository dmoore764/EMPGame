void CollisionUpdate(int goId)
{
	auto collider = GO(collider);
	auto tx = GO(transform);
	int collisionCount = 0;
	collider->collisions[0] = -1;
	collider->collisions[1] = -1;
	collider->collisions[2] = -1;
	collider->collisions[3] = -1;

	int minX = tx->pos.x + collider->bl.x;
	int minY = tx->pos.y + collider->bl.y;
	int maxX = tx->pos.x + collider->ur.x;
	int maxY = tx->pos.y + collider->ur.y;

	for (int i = 0; i < NumGameObjects; i++)
	{
		auto meta = &GameComponents.metadata[i];
		if (GameComponents.idIndex[i] != goId && (meta->cmpInUse & COLLIDER) && (meta->cmpInUse & TRANSFORM))
		{
			auto othCollider = &GameComponents.collider[i];
			auto othTx = &GameComponents.transform[i];
			int othMinX = othTx->pos.x + othCollider->bl.x;
			int othMinY = othTx->pos.y + othCollider->bl.y;
			int othMaxX = othTx->pos.x + othCollider->ur.x;
			int othMaxY = othTx->pos.y + othCollider->ur.y;
			
			if (minX <= othMaxX && maxX >= othMinX && minY <= othMaxY && maxY >= othMinY)
			{
				collider->collisions[collisionCount++] = GameComponents.idIndex[i];
				if (collisionCount == 4)
					return;
			}
		}
	}
}

void AnimationUpdate(int goId)
{
	auto anim = GO(anim);
	auto sprite = GO(sprite);

	anim->frameTime += (0.016f * anim->speedFactor);
	//check if we need to move to a new frame

	json_data_file *animFile = (json_data_file *)GetFromHash(&BasicAnimations, anim->name);
	int initialFrame = anim->currentFrame;
	if (anim->playing && animFile)
	{
		assert(animFile->val->type == JSON_ARRAY);
		int totalFrames = animFile->val->array->num_elements;
		if (anim->currentFrame > totalFrames - 1)
		{
			anim->currentFrame = 0;
			anim->frameTime = 0;
		}
		json_value *item = animFile->val->array->GetByIndex(anim->currentFrame);
		float t = GetJSONValAsFloat(item->hash->GetByKey("time"));
		while (anim->frameTime > t)
		{
			anim->frameTime -= t;
			if (anim->loop)
				anim->currentFrame = (anim->currentFrame + 1) % totalFrames;
			else
			{
				if (anim->currentFrame == totalFrames - 1)
				{
					anim->playing = false;
					break;
				}
				else
					anim->currentFrame++;
			}

			//get next json node
			if (anim->currentFrame == 0)
				item = animFile->val->array->first;
			else
				item = item->next;

			t = GetJSONValAsFloat(item->hash->GetByKey("time"));
		}

		//update the sprite component
		json_value *frame = animFile->val->array->GetByIndex(anim->currentFrame);
		assert(frame->type == JSON_HASH);
		json_value *sprName = frame->hash->GetByKey("sprite");
		assert(sprName->type == JSON_STRING);
		sprite->name = sprName->string;
	}
}
