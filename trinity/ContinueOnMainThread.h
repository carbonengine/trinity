////////////////////////////////////////////////////////////
//
//    Created:   October 2025
//    Copyright: CCP 2025
//

#pragma once

// Allows scheduling an action to be executed on the main thread.
// Useful for scenarios where certain operations cannot be performed
// on worker threads, like interacting with Python objects.
void ContinueOnMainThread( std::function<void()>&& action );

// Executes all actions that have been scheduled to run on the main thread.
// Resets the action queue after execution. Must be called from the main thread.
void ExecuteMainThreadActions();
