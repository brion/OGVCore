#include <functional>

namespace OGVCore {

	typedef std::function<void(int,int,int)> BisectorFunc;

	/**
	 * Give as your 'process' function something that will trigger an async
	 * operation, then call the left() or right() methods to run another
	 * iteration, bisecting to the given direction.
	 *
	 * Caller is responsible for determining when done.
	 *
	 * @params options object {
	 *   start: number,
	 *   end: number,
	 *   process: function(start, position, end)
	 * }
	 */
	class Bisector {
	private:
		int start_;
		int end_;
		BisectorFunc process_;
		int position_ = 0;
		int n_ = 0;

		void iterate() {
			n_++;
			position_ = (start_ + end_) / 2;
			process_(start_, end_, position_);
		}
	
	public:
		Bisector(int start, int end, BisectorFunc process):
			start_(start),
			end_(end),
			process_(process)
		{
		}

		void start() {
			iterate();
		}

		void left() {
			end_ = position_;
			iterate();
		}

		void right() {
			start_ = position_;
			iterate();
		}

	};

}
