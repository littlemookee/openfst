#ifndef SHM_MODELS_MANAGER_FST
#define SHM_MODELS_MANAGER_FST

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_upgradable_mutex.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>

#ifdef WIN32
	#define USE_WINDOWS_SHARED_MEMORY
#endif

#ifdef USE_WINDOWS_SHARED_MEMORY
	#include <boost/interprocess/windows_shared_memory.hpp>
	#include <boost/interprocess/managed_windows_shared_memory.hpp>

	typedef boost::interprocess::windows_shared_memory shm_object;
	typedef boost::interprocess::basic_managed_windows_shared_memory <char, boost::interprocess::rbtree_best_fit<boost::interprocess::mutex_family>,
				boost::interprocess::iset_index> managed_shm;
#else
	typedef boost::interprocess::shared_memory_object shm_object;
	typedef boost::interprocess::managed_shared_memory managed_shm;
#endif


//Менеджер блоков шаренной памяти
extern "C" {
	class ShmModelsManagerBase
	{
	public:
		//Обёртка над блоком памяти
		class ModelDesc{
		public:
			//Получить ссылку на даннее
			void * get_data() {return data;};

			//Получить размер блока данных
			std::size_t get_size() const;

			//имя памяти
			std::string get_name() const {return model_name;}

			//Детсруктор.
			~ModelDesc();

			//запрос мьютекса
			boost::interprocess::offset_ptr<boost::interprocess::interprocess_upgradable_mutex> get_mutex();

			//проверка памяти
			bool is_empty();

			//Создание памяти
			void create_memory(std::size_t data_size, const void * data=nullptr);
		private:
			friend class ShmModelsManagerBase;
			ShmModelsManagerBase & server;
			std::string model_name;

			void * data;

			//Обьект памяти
			std::shared_ptr<shm_object> memory;

			//мапинг памяти
			std::shared_ptr<boost::interprocess::mapped_region> region;

			//конструктор с подготовленной памятью
			ModelDesc(ShmModelsManagerBase & server, const std::string & model_name, const void * data,  std::size_t data_size);

			//конструктор без памяти
			ModelDesc(ShmModelsManagerBase & server, const std::string & model_name);
		};

		//Удалить описание.
		bool RemoveDesc(const std::string & modelName);

		//ссылка на модель
		typedef std::shared_ptr<ModelDesc> Model;

		ShmModelsManagerBase(const std::string & sh_mem_name);
		~ShmModelsManagerBase(void);

		//Возвращает описание буффера в шаренной памяти
		Model create_model(void * data, std::size_t data_size, const std::string & model_name);

		//Возвращает описание буффера в шаренной памяти, в качестве имени используется md5
		Model create_model(void * data, std::size_t data_size);

		//Создаёт описание модели без памяти
		Model create_model_empty(const std::string & model_name);

		bool is_in_memory(const std::string & model_name);

		static ShmModelsManagerBase & get_instance(const std::string & sh_mem_name="shared_vector_fst_manager");

		void add_memory_name(const std::string memory_name);

		void remove_memory_name(const std::string memory_name);

	private:
		//Описание типов для рабаты с шаренной памятью
		typedef boost::interprocess::allocator<char, managed_shm::segment_manager> CharAllocator;
		typedef boost::interprocess::basic_string<char, std::char_traits<char>, CharAllocator> SHMString;

		//!Шаренная строка
		class SHMStringType: public SHMString
		{
		public:
			SHMStringType(const std::string & str, const CharAllocator & alloc) : SHMString(str.data(), str.size(), alloc) {}
			operator std::string(){return std::string(data(), size());}
		};

		//! описание шаренной памяти модели,
		struct SHModelDesc
		{
			boost::interprocess::offset_ptr<boost::interprocess::interprocess_upgradable_mutex> mutex; //мьютекс для блокирования создания модели.
			SHMStringType hs_mem_name; //! имя памяти
			int ref_counter;		   //счётчик ссылок.
			std::size_t size;		   //размер памяти

			//конструктор
			SHModelDesc(const boost::interprocess::offset_ptr<boost::interprocess::interprocess_upgradable_mutex> & _mutex,
						const std::string & str, std::size_t _size,
						const CharAllocator & alloc):mutex(_mutex), hs_mem_name(str, alloc), ref_counter(1), size(_size){}
		};


		typedef std::pair<const SHMStringType, SHModelDesc> PairType;
		typedef boost::interprocess::allocator<PairType, managed_shm::segment_manager> PairAllocator;
		typedef boost::interprocess::map<SHMStringType, SHModelDesc, std::less<SHMStringType>, PairAllocator> MapPairType;

	private:
		//имя шаренной памяти
		std::string sh_mem_name;

		//хранит списки
		managed_shm segment;

		//Для блокировки
		boost::interprocess::interprocess_upgradable_mutex & mutex;

		//Содержит описание памятей.
		MapPairType & models_desc;

		//Количество моделей
		int GetModelCount();

		//Получить описание модели
		SHModelDesc * GetModelDesc(const std::string & modelName) const;

		//Добавить описание модели
		void AddDesc(const std::string & modelName, const std::string & sh_mem_name, std::size_t size);
	};
}
#endif //SHM_MODELS_MANAGER_FST
