#pragma once

// Macro include file.
#include "StrontiumPCH.h"

// Project includes.
#include "Core/ApplicationBase.h"
#include "Core/Logs.h"

// STL includes. Mutex for thread safety.
#include <mutex>
#include <functional>

namespace Strontium
{
  // Define an asset handle to distinguish when an internal handle is used or not.
  typedef std::string AssetHandle;

  class Shader;

  // A class to handle asset creation, destruction and file loading.
  template <class T>
  class AssetManager
  {
  public:
    ~AssetManager()
    { }

    static std::mutex assetMutex;

    // Get an asset manager instance.
    static AssetManager<T>* getManager(T* defaultAsset = nullptr)
    {
      std::lock_guard<std::mutex> guard(assetMutex);

      if (instance == nullptr)
      {
        instance = new AssetManager<T>(defaultAsset);
        return instance;
      }
      else
        return instance;
    }

    // Check to see if the map has an asset.
    bool hasAsset(const AssetHandle &handle)
    {
      return this->assetStorage.count(handle) > 0;
    }

    // Attach an asset to the manager.
    void attachAsset(const AssetHandle &handle, T* asset)
    {
      std::lock_guard<std::mutex> guard(assetMutex);

      if (!this->hasAsset(handle))
      {
        this->assetNames.push_back(handle);
        this->assetStorage.insert({ handle, Unique<T>(asset) });
      }
      else
      {
        this->assetStorage.erase(handle);

        auto loc = std::find(this->assetNames.begin(), this->assetNames.end(), handle);
        if (loc != this->assetNames.end())
          this->assetNames.erase(loc);

        this->assetNames.push_back(handle);
        this->assetStorage.insert({ handle, Unique<T>(asset) });
      }
    }

    // Get the asset reference.
    T* getAsset(const AssetHandle &handle)
    {
      if (handle == "None")
        return this->defaultAsset.get();

      if (this->hasAsset(handle))
        return this->assetStorage.at(handle).get();
      else
        return this->defaultAsset.get();
    }

    // Delete the asset.
    void deleteAsset(const AssetHandle &handle)
    {
      std::lock_guard<std::mutex> guard(assetMutex);

      this->assetStorage.erase(handle);

      auto loc = std::find(this->assetNames.begin(), this->assetNames.end(), handle);
      if (loc != this->assetNames.end())
        this->assetNames.erase(loc);
    }

    void setDefaultAsset(T* asset)
    {
      std::lock_guard<std::mutex> guard(assetMutex);

      this->defaultAsset.reset(asset);
    }

    void getDefaultAsset()
    {
      std::lock_guard<std::mutex> guard(assetMutex);

      return this->defaultAsset.get();
    }

    // Get a reference to the asset name storage.
    std::vector<AssetHandle>& getStorage() { return this->assetNames; }
  private:
    AssetManager(T* defaultAsset)
      : defaultAsset(defaultAsset)
    { }

    static AssetManager<T>* instance;

    std::unordered_map<AssetHandle, Unique<T>> assetStorage;
    std::vector<AssetHandle> assetNames;
    Unique<T> defaultAsset;
  };

  template <typename T>
  AssetManager<T>* AssetManager<T>::instance = nullptr;

  template <typename T>
  std::mutex AssetManager<T>::assetMutex;
}
