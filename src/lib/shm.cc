#include <fst/shm.h>
//#include <fst/md5.h>
#include <iomanip>

using namespace boost::interprocess;

ShmModelsManagerBase::ShmModelsManagerBase(const std::string & _sh_mem_name):
	sh_mem_name(_sh_mem_name),
	segment(open_or_create, _sh_mem_name.c_str(), 0xffffff),
	mutex(*segment.find_or_construct<interprocess_upgradable_mutex>("Mutex")[1]()),
	models_desc(*segment.find_or_construct<MapPairType>("models_desc")
			(std::less<SHMStringType>(), PairAllocator(segment.get_segment_manager())))
{
}


ShmModelsManagerBase::~ShmModelsManagerBase(void)
{
	#ifndef USE_WINDOWS_SHARED_MEMORY
		if (GetModelCount() == 0)
			shm_object::remove(sh_mem_name.c_str());
	#endif
}


ShmModelsManagerBase::SHModelDesc * ShmModelsManagerBase::GetModelDesc(const std::string & modelName) const
{
	//память существует.
	CharAllocator alloc_inst (segment.get_segment_manager());
	MapPairType::iterator ch_iter = models_desc.find(SHMStringType(modelName, alloc_inst));

    if ( ch_iter != models_desc.end() )
		return &ch_iter->second;

	return NULL;
}

int ShmModelsManagerBase::GetModelCount()
{
	return models_desc.size();
}

bool ShmModelsManagerBase::RemoveDesc(const std::string & model_name)
{
	scoped_lock<interprocess_upgradable_mutex> lock(mutex);

	SHModelDesc & mem_desc = *GetModelDesc(model_name);
	mem_desc.ref_counter--;

	if ( mem_desc.ref_counter == 0 )
	{
		//Удалим мьютекс.
		segment.destroy_ptr(mem_desc.mutex.get());

		//Удалим память
		#ifndef USE_WINDOWS_SHARED_MEMORY
			shm_object::remove(mem_desc.hs_mem_name.c_str());
		#endif

			//Удалим данные из мапа.
		CharAllocator alloc_inst (segment.get_segment_manager());
		MapPairType::iterator iter = models_desc.find(SHMStringType(model_name, alloc_inst));
		models_desc.erase(iter);
		return true;
	}
	return false;
}

ShmModelsManagerBase & ShmModelsManagerBase::get_instance(const std::string & sh_mem_name)
{
	static std::map<std::string, std::shared_ptr<ShmModelsManagerBase> > managers;
	std::map<std::string , std::shared_ptr<ShmModelsManagerBase> >::iterator iter = managers.find(sh_mem_name);

	if (iter != managers.end())
		return *iter->second;
	else {
		return *managers.insert(std::pair<std::string, std::shared_ptr<ShmModelsManagerBase> >(
			sh_mem_name, std::shared_ptr<ShmModelsManagerBase>(new ShmModelsManagerBase(sh_mem_name)))).first->second;
	}
}


void ShmModelsManagerBase::AddDesc(const std::string & model_name, const std::string & sh_mem_name, std::size_t size)
{
	CharAllocator alloc_inst(segment.get_segment_manager());
	boost::interprocess::offset_ptr<boost::interprocess::interprocess_upgradable_mutex> mutex(segment.construct<boost::interprocess::interprocess_upgradable_mutex>(anonymous_instance)());
	MapPairType::iterator iter =  models_desc.insert(PairType(SHMStringType(model_name, alloc_inst), SHModelDesc(mutex, sh_mem_name, size, alloc_inst))).first;
}
/*
ShmModelsManagerBase::Model ShmModelsManagerBase::create_model(void * data, std::size_t data_size)
{
	unsigned char output[16]= {0};
	md5((unsigned char *)data, data_size, output);
	std::stringstream sstream;

	for (int i = 0; i < sizeof(output)/ sizeof(output[0]); i++) {
		sstream << std::right << std::setfill('0') << std::setw(2) << std::hex << int(output[i]);
	}
	std::string s = sstream.str();
	return create_model(data, data_size, s);
}
*/

ShmModelsManagerBase::Model ShmModelsManagerBase::create_model(void * data, std::size_t data_size, const std::string & model_name)
{
	scoped_lock<interprocess_upgradable_mutex> lock(mutex);
	return Model(new ModelDesc(*this, model_name, data, data_size));
}


bool ShmModelsManagerBase::is_in_memory(const std::string & model_name) {
	SHModelDesc * mem_desc = this->GetModelDesc(model_name);
	return  mem_desc && (!mem_desc->hs_mem_name.empty());
}


ShmModelsManagerBase::Model ShmModelsManagerBase::create_model_empty(const std::string & model_name) {
	scoped_lock<interprocess_upgradable_mutex> lock(mutex);
	return Model(new ModelDesc(*this, model_name));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

ShmModelsManagerBase::ModelDesc::ModelDesc(ShmModelsManagerBase & _server, const std::string & _model_name, const void * _data,  std::size_t _data_size):
	server(_server),
	data(NULL),
	model_name(_model_name)
{
	//память существует и модель уже развёрнута!!!!!!!!!!
	if( SHModelDesc * mem_desc = server.GetModelDesc(model_name) ) {
		memory = std::shared_ptr<shm_object>(new shm_object(open_only, mem_desc->hs_mem_name.c_str(), read_only));
		region = std::shared_ptr<mapped_region>(new mapped_region(*memory, read_only));

		mem_desc->ref_counter++;
		data = region->get_address();
		//грузим модель.
	} else {
		std::stringstream ___ss; ___ss << server.sh_mem_name << "_"<< server.GetModelCount();
		std::string mem_name = ___ss.str();

		#ifndef USE_WINDOWS_SHARED_MEMORY
			shared_memory_object::remove(mem_name.c_str());
			memory = std::shared_ptr<shm_object>(new shm_object(open_or_create, mem_name.c_str(), read_write));
			memory->truncate(_data_size);
		#else
			memory = std::shared_ptr<shm_object>(new shm_object(open_or_create, mem_name.c_str(), read_write, _data_size));
		#endif

		region = std::shared_ptr<mapped_region >(new mapped_region(*memory, read_write));
		data = (char *)region->get_address();
		memcpy(data, _data, _data_size);
		server.AddDesc(model_name, mem_name, _data_size);
	}
}

//конструктор без памяти
ShmModelsManagerBase::ModelDesc::ModelDesc(ShmModelsManagerBase & _server, const std::string & _model_name):
	server(_server),
	data(NULL),
	model_name(_model_name)
{
	//память существует и модель уже развёрнута!!!!!!!!!!
	if( SHModelDesc * mem_desc = server.GetModelDesc(model_name) ) {
		//описание в данный момент в режиме инициализации данных
		if ( !mem_desc->hs_mem_name.empty() )
		{
			memory = std::shared_ptr<shm_object>(new shm_object(open_only, mem_desc->hs_mem_name.c_str(), read_only));
			region = std::shared_ptr<mapped_region>(new mapped_region(*memory, read_only));
			data = region->get_address();
		}

		mem_desc->ref_counter++;
	//Добавим описание в мап
	} else {
		server.AddDesc(model_name, "", 0);
	}
}



//Получить размер блока данных
std::size_t ShmModelsManagerBase::ModelDesc::get_size() const {
	return server.GetModelDesc(model_name)->size;
}

ShmModelsManagerBase::ModelDesc::~ModelDesc()
{
	//Чистим память и тп.
	if ( memory ) {
		region = std::shared_ptr<boost::interprocess::mapped_region>();
		memory = std::shared_ptr<shm_object>();

		server.RemoveDesc(model_name);
	}
}

//запрос мьютекса
boost::interprocess::offset_ptr<boost::interprocess::interprocess_upgradable_mutex> ShmModelsManagerBase::ModelDesc::get_mutex() {
	return server.GetModelDesc(model_name)->mutex;
}

//проверка памяти
bool ShmModelsManagerBase::ModelDesc::is_empty() {
	scoped_lock<interprocess_upgradable_mutex> lock(server.mutex);
	SHModelDesc * mem_desc = server.GetModelDesc(model_name);

	bool is_empty = mem_desc->hs_mem_name.empty();

	if (!is_empty && !memory) {
		memory = std::shared_ptr<shm_object>(new shm_object(open_only, mem_desc->hs_mem_name.c_str(), read_only));
		region = std::shared_ptr<mapped_region>(new mapped_region(*memory, read_only));
		data = region->get_address();
	}
	return is_empty;
}

//Создание памяти
void ShmModelsManagerBase::ModelDesc::create_memory(std::size_t _data_size, const void * _data) {
	std::stringstream ___ss; ___ss << server.sh_mem_name << "_"<< model_name;
	std::string mem_name = ___ss.str();

	#ifndef USE_WINDOWS_SHARED_MEMORY
		memory = std::shared_ptr<shm_object>(new shm_object(create_only, mem_name.c_str(), read_write));
		memory->truncate(_data_size);
	#else
		memory = std::shared_ptr<shm_object>(new shm_object(create_only, mem_name.c_str(), read_write, _data_size));
	#endif

	region = std::shared_ptr<mapped_region >(new mapped_region(*memory, read_write));

	data = (char *)region->get_address();

	//заполним данные
	if (_data != NULL ) {
		memcpy(data, _data, _data_size);
	}
	SHModelDesc * desc(server.GetModelDesc(model_name));

	//Обновим данные
	scoped_lock<interprocess_upgradable_mutex> lock(server.mutex);

	desc->hs_mem_name.assign(mem_name.c_str(), mem_name.size());
	desc->size = _data_size;
}
