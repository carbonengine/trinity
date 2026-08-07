// Copyright © 2026 CCP ehf.

#pragma once

#include "IEveSpaceObjectChild.h"

BLUE_CLASS_IMPL( EveSpaceObjectChild );

/**
 * @brief Base class for all space object children. This class provides common functionality and properties for all child objects in the space object hierarchy.
 */
class EveSpaceObjectChild : public IEveSpaceObjectChild
{
public:
	EXPOSE_TO_BLUE();

	/// @brief Enum representing the origin of the space object child, i.e. how it was created: using SOF or otherwise.
	enum Origin
	{
		SPACE,
		SOF,
	};

	/// @brief Type representing a part tag, which is used to identify parts of a modular space object.
	using PartTag = uint32_t;

	/// @brief Constant representing a part tag for children that are not part of a modular space object.
	static const PartTag NO_PART_TAG = 0;

	EveSpaceObjectChild( IRoot* lockobj = nullptr );

	/**
	 * @brief Returns the name of the space object child. This name is used for identification and debugging purposes.
	 */
	const char* GetName() const;

	/**
	 * @brief Sets the name of the space object child. This name is used for identification and debugging purposes.
	 * @param name The new name to set for the space object child.
	 */
	virtual void SetName( const char* name );

	/**
	 * @brief Updates the internal visibility state (frustum culling, LOD, etc.) of the object. This method is called during rendering before GetRenderables() to determine if the object should be rendered.
	 * Note, that this method is called from worker threads, so care should be taken when accessing shared data.
	 * The default implementation does nothing, derived classes should override this method to implement visibility logic if needed.
	 * @param updateContext Scene-wide update context containing timing and other relevant information for the update.
	 * @param parentTransform Parent transformation matrix to world coordinates.
	 * @param parentLod Owner space object's LOD level.
	 */
	virtual void UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod );

	/**
	 * @brief Pushes owned renderables to the provided vector. This method is called during rendering to collect all renderable objects for the current frame.
	 * The default implementation does nothing, derived classes implementing renderable features should override this method to provide their renderables.
	 * @param renderables Vector to which the renderables will be added.
	 */
	virtual void GetRenderables( std::vector<ITr2Renderable*>& renderables );

	/**
	 * @brief Retrieves the bounding sphere of the object. This method is used for frustum culling and LOD estimation.
	 * The default implementation does nothing, derived classes should override this method to provide their bounding sphere if available.
	 * @param sphere Vector4 to store the bounding sphere (x, y, z for center, w for radius).
	 * @param query Type of bounding sphere query.
	 * @return True if the bounding sphere is valid, false otherwise.
	 */
	virtual bool GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const;

	/**
	 * @brief Registers effects with the quad renderer. This method is called when the child is added to the scene to allow it to register any effects that need to be rendered as quads.
	 * The default implementation does nothing, derived classes implementing quad rendering should override this method to register their effects with the quad renderer.
	 * @param quadRenderer Quad renderer to register effects with.
	 */
	virtual void RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer );

	/**
	 * @brief Adds quads to the quad renderer for rendering. This method is called during rendering to allow the child to add any quads that need to be rendered.
	 * Note, that this method is called from worker threads, so care should be taken when accessing shared data.
	 * The default implementation does nothing, derived classes implementing quad rendering should override this method to add their quads to the quad renderer.
	 * @param frustum Frustum used for culling and visibility checks.
	 * @param quadRenderer Quad renderer to which quads will be added.
	 */
	virtual void AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const;

	/**
	 * @brief Updates the state of the object. This method is called every frame from the main thread to allow the child to update its internal state, animations, etc.
	 * The default implementation does nothing, derived classes implementing quad rendering should override this method to add their quads to the quad renderer.
	 * @param updateContext Scene-wide update context containing timing and other relevant information for the update.
	 * @param params Space object and parent child information relevant for the update.
	 */
	virtual void UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );

	/**
	 * @brief Updates the state of the object asynchronously. This method is called from worker threads to allow the child to update its internal state, animations, etc.
	 * The default implementation does nothing, derived classes implementing quad rendering should override this method to add their quads to the quad renderer.
	 * @param updateContext Scene-wide update context containing timing and other relevant information for the update.
	 * @param params Space object and parent child information relevant for the update.
	 */
	virtual void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params );

	/**
	 * @brief Retrieves the local to world transformation matrix of the object. This method is poorly designed. Avoid using it if possible. Not every child has a local 
	 * to world transformation, and this method has no way to return the failure state. The default implementation does nothing, derived classes should override this 
	 * method to provide their local to world transformation if available.
	 * @param transform Matrix to store the local to world transformation.
	 */
	virtual void GetLocalToWorldTransform( Matrix& transform ) const;

	/**
	 * @brief Exotic method that is used to determine if the child needs to be always updated/rendered regardless of the owner's space object "activation strength".
	 * @return True if the child should always be updated/rendered, false otherwise.
	 */
	virtual bool IsAlwaysOn() const;

	/**
	 * @brief Sets up the child transform and LOD. This methods is called by the SOF when creating a child object.
	 * The default implementation does nothing. If the child has a transform, it should override this method to set up the transform and LOD.
	 * @param scale Optional scale factors (x, y, z) for the child transform. If nullptr, scaling is unaffected.
	 * @param rotation Optional rotation for the child transform. If nullptr, rotation is unaffected.
	 * @param translation Optional translation for the child transform. If nullptr, translation is unaffected.
	 * @param lowestLodVisible Lowest level of detail (LOD) of the owner space object that is visible for the child.
	 */
	virtual void Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible );

	/**
	 * @brief Notifies the child object that the level of detail (LOD) of the owner space object has changed.
	 * The default implementation does nothing. If the child has LOD-dependent behavior, it should override this method to respond to LOD changes.
	 * @param lod New level of detail (LOD) of the owner space object.
	 */
	virtual void ChangeLOD( Tr2Lod lod );

	/**
	 * @brief Sets a controller variable for the child object. This method is called by the owner space object to update controller variables that affect the child's behavior.
	 * If the child owns controllers, it should override this method to propagate the variable to its controllers. Also, if the object owns other children, it should propagate the 
	 * controller variable change to them as well.
	 * The default implementation does nothing.
	 * @param name Name of the controller variable.
	 * @param value Value to set for the controller variable.
	 */
	virtual void SetControllerVariable( const char* name, float value );

	/**
	 * @brief Notifies child controllers that a controller event has occurred. This method is called by the owner space object to notify the child of controller events that may affect its behavior.
	 * If the child owns controllers, it should override this method to propagate the event to its controllers. Also, if the object owns other children, it should propagate the 
	 * controller event to them as well.
	 * The default implementation does nothing.
	 * @param name Name of the controller event.
	 */
	virtual void HandleControllerEvent( const char* name );

	/**
	 * @brief Starts controllers owned by the child object. If the child owns controllers, it should override this method to start them. Also, if the object owns other children, it should propagate the
	 * start command to them as well.
	 * The default implementation does nothing.
	 */
	virtual void StartControllers();

	/**
	 * @brief Sets a "procedural container" variable. This method is very specific to EveChildProceduralContainer. Derived classes that own other children should override this method to propagate the call to them.
	 * The default implementation does nothing.
	 * @param name Name of the procedural container variable.
	 * @param value Value to set for the procedural container variable.
	 */
	virtual void SetProceduralContainerVariable( const char* name, float value );

	/**
	 * @brief Sets a shader option for effects managed by the child object. Derived classes that manage effects (or meshes) should override this methods to set the shader option for their effects. Also, if the object
	 * owns other children, it should override this method to propagate the call to them.
	 * The default implementation does nothing.
	 * @param name Name of the shader option.
	 * @param value Value to set for the shader option.
	 */
	virtual void SetShaderOption( const BlueSharedString& name, const BlueSharedString& value );

	/**
	 * @brief Sets the origin (creation method) of the child object. This method is called by the SOF to set the creation method of the object.
	 * The default implementation does nothing.
	 * @param origin Origin (creation method) of the child object.
	 */
	virtual void SetOrigin( Origin origin );

	/**
	 * @brief Adds a transform modifier to the child object. If the child object supports transform modifiers, it can override this method to add the modifier to its list of modifiers. 
	 * The default implementation does nothing.
	 * @param modifier New transform modifier.
	 */
	virtual void AddTransformModifier( IEveChildTransformModifier* modifier );

	/**
	 * @brief Sets the muted state for audio emitters owned by the child object. If the child object owns audio emitters, it can override this method to set their muted state. 
	 * Also, if the object owns other children, it should override this method to propagate the call to them.
	 * The default implementation does nothing.
	 * @param isMuted Should the audio emitters be muted (true) or unmuted (false).
	 */
	virtual void SetMute( bool isMuted );

	/**
	 * @brief Returns the part tag of the child object for multi-part space objects.
	 * @return Part tag of the child object, or NO_PART_TAG if not applicable.
	 */
	PartTag GetPartTag() const;

	/**
	 * @brief Sets the part tag of the child object for multi-part space objects. Descendent classes that own other children should override this method to propagate the part tag to them.
	 * @param tag Part tag to set for the child object.
	 */
	virtual void SetPartTag( PartTag tag );

	/**
	 * @brief Returns the owner space object of the child. It may return nullptr if the child is not currently attached to an owner space object.
	 * @return Owner space object of the child, or nullptr if not attached.
	 */
	IEveSpaceObject2* GetOwner() const;

	/**
	 * @brief Sets the owner space object of the child. Descendent classes that own other children should override this method to propagate the owner to them.
	 * @param owner Owner space object to set for the child.
	 */
	virtual void SetOwner( IEveSpaceObject2* owner );

	/**
	 * @brief Returns the parent child object of this child. It may return nullptr if the child is not currently attached to a parent child object.
	 * @return Parent child object of this child, or nullptr if not attached.
	 */
	EveSpaceObjectChild* GetParent() const;

	/**
	 * @brief Sets the parent child object of this child.
	 * @param parent Parent child object to set for this child.
	 */
	void SetParent( EveSpaceObjectChild* parent );

protected:
	/**
	 * @brief Helper function that registers a child object with this parent. Assigns the parent, owner and part tag to the child.
	 * @param child Child object to register. If the child is nullptr, the method does nothing.
	 */
	void RegisterChild( EveSpaceObjectChild* child );

	/**
	 * @brief Helper function that unregisters a child object with this parent. Resets the parent and owner pointers of the child.
	 * @param child Child object to unregister. If the child is nullptr, the method does nothing.
	 */
	void UnregisterChild( EveSpaceObjectChild* child );

	/**
	 * @brief Helper function that registers a sequence of child objects with this parent. Assigns the parent, owner and part tag to each child.
	 * @param children Sequence of child objects to register.
	 */
	template <typename ChildList>
	void RegisterChildren( const ChildList& children )
	{
		for( auto& child : children )
		{
			this->RegisterChild( child );
		}
	}

	/**
	 * @brief Helper function that unregisters a sequence of child objects with this parent. Resets the parent and owner pointers of each child.
	 * @param children Sequence of child objects to unregister.
	 */
	template <typename ChildList>
	void UnregisterChildren( const ChildList& children )
	{
		for( auto& child : children )
		{
			this->UnregisterChild( child );
		}
	}

	/**
	 * @brief Helper function that handles list modification notifications for a list of child objects to register and unregister children.
	 * @tparam ChildList Type of the list of child objects.
	 * @param event Event type indicating the modification. See BLUELISTEVENT for possible values.
	 * @param value Value associated with the event.
	 * @param children List of child objects for which the notification is being sent.
	 */
	template <typename ChildList>
	void HandleChildrenListModified(
		long event, // BLUELISTEVENT values
		IRoot* value,
		const ChildList& children )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( EveSpaceObjectChildPtr child = BlueCastPtr( value ) )
			{
				this->RegisterChild( child );
			}
			break;
		case BELIST_REMOVED:
			if( EveSpaceObjectChildPtr child = BlueCastPtr( value ) )
			{
				this->UnregisterChild( child );
			}
			break;
		case BELIST_UNLOADSTART:
			for( auto& child : children )
			{
				this->UnregisterChild( child );
			}
			break;
		default:
			break;
		}
	}

	BlueSharedString m_name;

private:
	IEveSpaceObject2* m_owner = nullptr;
	EveSpaceObjectChild* m_parent = nullptr;
	PartTag m_partTag = NO_PART_TAG;
};

BLUE_DECLARE_VECTOR( EveSpaceObjectChild );

TYPEDEF_BLUECLASS( EveSpaceObjectChild );