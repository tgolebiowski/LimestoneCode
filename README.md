The LimestoneCode Framework is a collection of C/C++ files designed for the development of creative, interactive software, with an emphasis on the use of 3D graphics.
Project Ideals:
	-Keep code maybe a little simpler than nescessary. Every component should be easy to delete and/or redo in a fashion that better serves the problem at hand. Individuals stand more to gain from tools that they design to work for exactly the problems they encounter, no more no less and LimestoneCode aims to be the raw starting point for arriving at such a solution, rather than a finished pre-designed solution.
	-Do not just create another competitor to Unity or Unreal. These are massive engines filled with general case solutions. A framework that allows a programmer to leverage domain knowledge of a problem should provide more productivity boons than overly complex systems that try to cater to every case.

The project is still very much in its infancy but currently has the following features:
	- Collada File loading & parsing
	    *With armature/skeleton loading & parsing
	- A simple OpenGL renderer, with exposed data structures for shader programs and draw call parameters.
	- WAV file loading and playback queueing.
	- Mouse, Keyboard, and Controller support.
	- A Manual Memory management system.
	- 3D math
	- I would mention image loading but i'm just using stbi_image, so credit is due there.
	- Non-platform code hot reloading
	- A general system for detecting new assets.

Major but vague "ToDos"
	- Remove all uses of malloc and switch over to only using internal memory system.
	- So far, nothing has any member functions, its only structs and functions.  However, there are places where converting existing functions to member functions would improve readability and simplify the existing code without introducing any new complexity.
	- Renderer unesscessairly relies on a "god" state structure in places where data should be decoupled from this governing struct.
	- Set up a system for manually including modules or non-core components, rather than creating a monolithic kind of framework like Unity or Unreal do where every feature is available whether you want it or not.
	- Logging should stop using printf, use an internal thing that will use printf by default but can be used to switch to some other logger.
	- No SIMD use yet. Anywhere. Must be fixed.