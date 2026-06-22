// Copyright © 2023 CCP ehf.

#pragma once

class Tr2SyncToGpu
{
public:
	template <typename Task>
	void SyncToGpu( Task task )
	{
		m_tasks.push_back( { task, m_frame } );
	}

	static Tr2SyncToGpu& GetInstance();

	void Tick();
	void Flush();

private:
	struct Task
	{
		std::function<void()> task;
		uint64_t frame;
	};
	std::vector<Task> m_tasks;
	uint64_t m_frame = 0;
};

template <typename Task>
void SyncToGpu( Task task )
{
	Tr2SyncToGpu::GetInstance().SyncToGpu( task );
}
