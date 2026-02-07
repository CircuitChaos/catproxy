#include "model.h"
#include "model_ft891.h"

ModelList getModelList()
{
	ModelList rs;
	rs[ModelFT891::getName()] = ModelFT891::getMeterList();
	return rs;
}

Model *createModel(const std::string &name)
{
	if(name == ModelFT891::getName()) {
		return new ModelFT891();
	}

	return nullptr;
}
