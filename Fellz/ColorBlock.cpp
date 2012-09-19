#include "ColorBlock.h"


USING_NS_CC;

std::list<ColorBlock*> ColorBlock::blocksToBeDeleted;

ColorBlock::ColorBlock()
{
	body = NULL;
	world = NULL;

	// generates a random number that'll define the cube color
	//cubeColor = rand() % 3;
	cubeColor = 0;

	isDying = false;
}

ColorBlock* ColorBlock::create(const char * file)
{
	// I hope that I made the corret init sequence

	ColorBlock *pobSprite = new ColorBlock();
    if (pobSprite && pobSprite->initWithFile(file))
    {
        pobSprite->autorelease();
        return pobSprite;
    }
    CC_SAFE_DELETE(pobSprite);
    return NULL;
}

void ColorBlock::update(float dt)
{
	// update sprite's position based on body from Box2D
	this->setPosition(ccp(body->GetPosition().x * PTM_RATIO, body->GetPosition().y * PTM_RATIO));
	this->setRotation(-1 * CC_RADIANS_TO_DEGREES(body->GetAngle()));
}
void ColorBlock::InitInWorld(b2World* world)
{

	// put the update method to work;
	this->scheduleUpdate();

	// hold reference
	this->world = world;

	// create body stuff here

	// descriptor
	b2BodyDef blockDef;
	blockDef.type = b2_dynamicBody;
	blockDef.allowSleep = false;
	// divide by the PTM ratio
	blockDef.position.Set(this->getPositionX() / PTM_RATIO, this->getPositionY() / PTM_RATIO);
	// define that this CCSprite is linked with this body
	blockDef.userData = this;
	// creates body with this descriptors
	body = world->CreateBody(&blockDef);

	// now comes the shape
	b2PolygonShape blockShape;
	// create shape based on the size of the sprite
	blockShape.SetAsBox((this->boundingBox().size.width / PTM_RATIO)/2,(this->boundingBox().size.height / PTM_RATIO)/2);
	
	// connect shape with body
	b2FixtureDef blockFixture;
	blockFixture.shape = &blockShape;
	blockFixture.density = 1.0f;
	blockFixture.friction = 0.2f;
	blockFixture.restitution = 0.8f;

	// add to body
	body->CreateFixture(&blockFixture);

	// make the blocks goes up
	body->SetLinearVelocity(b2Vec2(0.0,3.0f));

	// all blocks are initialized detached
	attached = false;

	// set tag
	this->setTag(BLOCK_TAG);

	// define color based on random number
	switch (cubeColor)
	{
	case BLOCK_BLUE:
		// blue color
		this->setColor(ccc3(0,0,255));
		break;
	case BLOCK_GREEN:
		// green color
		this->setColor(ccc3(0,255,0));
		break;
	case BLOCK_RED:
		// green color
		this->setColor(ccc3(255,0,0));
		break;
	}
}

ColorBlock::~ColorBlock(void)
{
	if (body != NULL)
	{
		world->DestroyBody(body);
	}
}

void ColorBlock::Destroy()
{
	isDying = true;
	// put a fade in the block, will last 0.5 seconds
	CCFadeOut* fade = CCFadeOut::actionWithDuration(0.5f);
	this->runAction(fade);

	Detach();
}

void ColorBlock::AttachTo(b2Body* toAttach)
{
	// set this flag with true
	attached = true;

	
	b2Vec2 worldCoordsAnchorPoint = body->GetWorldPoint(b2Vec2(0.5f,0));
	// define joint
	b2WeldJointDef jointDef;
	jointDef.bodyA = toAttach;
	jointDef.bodyB = body;
	jointDef.localAnchorB = body->GetLocalPoint(worldCoordsAnchorPoint);
	jointDef.localAnchorA = toAttach->GetLocalPoint(worldCoordsAnchorPoint);
	jointDef.referenceAngle = body->GetAngle() - toAttach->GetAngle();
	jointDef.collideConnected = true; // to prevent collisions from stoping ocurring
	// add to world
	world->CreateJoint(&jointDef);

	// update list of atached blocks
	if (((CCSprite*)toAttach->GetUserData())->getTag() == BLOCK_TAG)
	{
		((ColorBlock*)toAttach->GetUserData())->attachedBlocks.push_back(this);
	}

	
}

void ColorBlock::BuildConnections(const ColorBlock* caller,const int blockType)
{
	int colorBlock_s;
	b2Body* collideBody;
	b2ContactEdge* edge = body->GetContactList();
	// must find at least three adjacents blocks attacheds to this one
	// first check if it's the Scene that it's calling
	if (caller == NULL)
	{
		// check which type of block we need to search
		colorBlock_s = cubeColor;
		while(edge != NULL)
		{
			// must first check if the block is dying
			if (!((ColorBlock*)edge->contact->GetFixtureA()->GetBody()->GetUserData())->GetDying() &&
				!((ColorBlock*)edge->contact->GetFixtureB()->GetBody()->GetUserData())->GetDying())
			{
				// hold the two first objects in the first iteration
				// I need to know which fixture holds the current block and which holds the adjacent block
				if (edge->contact->GetFixtureA()->GetBody() == body)
				{
					collideBody = edge->contact->GetFixtureB()->GetBody();
				}
				else
				{
					collideBody = edge->contact->GetFixtureA()->GetBody();
				}
				if(((CCSprite*)collideBody->GetUserData())->getTag() == BLOCK_TAG)
				{
					// check if it's attached
					if (((ColorBlock*)collideBody->GetUserData())->GetAttached())
					{
						if (((ColorBlock*)collideBody->GetUserData())->GetBlockColor() == colorBlock_s)
						{
							// ok, there's a adjacent block, must store reference to it and call build connections on them
							// add both objects on the list to be destroyed
							blocksToBeDeleted.push_back(this);
							blocksToBeDeleted.push_back((ColorBlock*)collideBody->GetUserData());
							// build connections on the next
							((ColorBlock*)collideBody->GetUserData())->BuildConnections(this,colorBlock_s);
						}
					}
				}
			}
			edge = edge->next;
		}
		// after all function returns, see if there's enough blocks to create a block anihilation
		if (blocksToBeDeleted.size() > 2)
		{
			// iterate through blocks calling destroy
			std::list<ColorBlock*>::iterator it;
			//std::list<ColorBlock*>::iterator it2;
			//for (it = blocksToBeDeleted.begin(); it != blocksToBeDeleted.end(); it++)
			//{
			//	for (it2 = it,it2++; it2 != blocksToBeDeleted.end(); it2++)
			//	{
			//		if ((*it) == (*it2))
			//		{
			//			int b = 0;
			//		}
			//	}
			//}
			for (it = blocksToBeDeleted.begin(); it != blocksToBeDeleted.end(); it++)
			{
				(*it)->Destroy();
			}
		}
		//...anyway, erase all content of the list
		blocksToBeDeleted.clear();
	}
	// now process the case when the funcion is called by the other blocks
	else
	{
		colorBlock_s = blockType;
		while (edge != NULL)
		{
			// must first check if the block is dying
			if (!((ColorBlock*)edge->contact->GetFixtureA()->GetBody()->GetUserData())->GetDying() &&
				!((ColorBlock*)edge->contact->GetFixtureB()->GetBody()->GetUserData())->GetDying())
			{
				// rule out case when we're processing the collision betwen this block and the caller
				if (edge->contact->GetFixtureA()->GetBody()->GetUserData() != caller && 
					edge->contact->GetFixtureB()->GetBody()->GetUserData() != caller)
				{
					// I need to know which fixture holds the current block and which holds the adjacent block
					if (edge->contact->GetFixtureA()->GetBody() == body)
					{
						collideBody = edge->contact->GetFixtureB()->GetBody();
					}
					else
					{
						collideBody = edge->contact->GetFixtureA()->GetBody();
					}
					// make sure it's a block
					if(((CCSprite*)collideBody->GetUserData())->getTag() == BLOCK_TAG)
					{
						// make sure it's attached
						if (((ColorBlock*)collideBody->GetUserData())->GetAttached())
						{
							if (((ColorBlock*)collideBody->GetUserData())->GetBlockColor() == colorBlock_s)
							{
								// search for this current block on blocks list
								std::list<ColorBlock*>::iterator r_it;
								bool dontAdd = false;
								for (r_it = blocksToBeDeleted.begin(); r_it != blocksToBeDeleted.end();r_it++ )
								{
									if ((*r_it) == ((ColorBlock*)collideBody->GetUserData()))
									{
										// if its already there, don't add
										dontAdd = true;
										break;
									}
								}
								if (!dontAdd)
								{
									// ok, there's a adjacent block, must store reference to it and call build connections on it
									blocksToBeDeleted.push_back((ColorBlock*)collideBody->GetUserData());
									// build connections on the next
									((ColorBlock*)collideBody->GetUserData())->BuildConnections(this,colorBlock_s);
								}
							}
						}
					}
				}
			}
			edge = edge->next;
		}
	}
}

void ColorBlock::Detach()
{
	// iterate through the list of joints
	b2JointEdge* joints = body->GetJointList();

	// destroy all the joints in this object
	while(joints != NULL)
	{
		b2JointEdge* next = joints->next;
		world->DestroyJoint(joints->joint);
		joints = next;
	}

	attached = false;

	// iterate through the list of "childs"
	std::list<ColorBlock*>::iterator it;
	for (it = attachedBlocks.begin(); it != attachedBlocks.end(); it++)
	{
		// call detach on each one of them
		(*it)->Detach();
	}
	attachedBlocks.clear();
}
