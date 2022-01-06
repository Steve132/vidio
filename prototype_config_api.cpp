#ifndef PROTOTYPE_CONFIG_API_HPP
#define PROTOTYPE_CONFIG_API_HPP


#include<chrono>

namespace vidio
{

using timestamp=std::chrono::nanoseconds;
using duration=std::chrono::nanoseconds;



namespace info
{
	struct Metaline:
		public std::pair<std::string,std::string>{
		Metaline(const std::string& k,const std::string& v);
	};

	struct Metadata:
		public std::vector<Metaline>
	{
	public:
		using std::vector<Metaline>::operator[];
		const Metaline& operator[](const std::string&) const;
		Metaline& operator[](const std::string&);
	private:
		std::unordered_map<std::string,size_t> lookup;
	};

	struct Stream: public Metadata{
		unsigned int index;

	};
	struct Chapter: public Metadata{

	};

	struct Input
	{
	private:
		using StreamRef=std::reference_wrapper<Stream>;
	public:
		std::vector<Stream> streams;
		std::vector<Chapter> chapters;
	};
	struct Data{
		std::vector<Input> inputs;
	};
}


}

#endif // PROTOTYPE_CONFIG_API_HPP
