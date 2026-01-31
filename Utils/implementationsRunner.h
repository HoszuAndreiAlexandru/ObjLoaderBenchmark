#pragma once
#include "../types.h"

#include "../Implementations/loader_template.h"
#include "../Implementations/naive.h"
#include "../Implementations/own_fast.h"
#include "../Implementations/tiny_obj_loader.h"
#include "../Implementations/fast_obj.h"

std::vector<LoaderTemplate*>& GetRegistry()
{
	static std::vector<LoaderTemplate*> registry;
	return registry;
}

struct Registrar
{
	Registrar(LoaderTemplate* instance)
	{
		GetRegistry().push_back(instance);
	}
};

static Naive naiveImplementation;
static Registrar registerA(&naiveImplementation);

static OwnFast ownFastImplementation;
static Registrar registerB(&ownFastImplementation);

static TinyObjLoader tinyObjLoaderImplementation;
static Registrar registerC(&tinyObjLoaderImplementation);

static FastObj fastObjImplementation;
static Registrar registerD(&fastObjImplementation);

std::vector<Results> runImplementations(const std::vector<std::string> paths)
{
	std::vector<Results> results{};

	for (LoaderTemplate* p : GetRegistry())
	{
		std::cout << "\nRunning: " << p->Name() << "\n\n";
		results.push_back({ p->Name(), p->loadAllObjs(paths) });
	}

	return results;
};