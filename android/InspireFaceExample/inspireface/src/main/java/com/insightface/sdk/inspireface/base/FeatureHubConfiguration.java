package com.insightface.sdk.inspireface.base;

public class FeatureHubConfiguration {
    public int primaryKeyMode = 0;
    public int enablePersistence = 0;
    public String persistenceDbPath = "";
    public float searchThreshold = 0.42f;
    public int searchMode = 0;

    public FeatureHubConfiguration setPrimaryKeyMode(int primaryKeyMode) {
        this.primaryKeyMode = primaryKeyMode;
        return this;
    }

    public FeatureHubConfiguration setEnablePersistence(boolean enablePersistence) {
        this.enablePersistence = enablePersistence ? 1 : 0;
        return this;
    }

    public FeatureHubConfiguration setPersistenceDbPath(String persistenceDbPath) {
        this.persistenceDbPath = persistenceDbPath;
        return this;
    }

    public FeatureHubConfiguration setSearchThreshold(float searchThreshold) {
        this.searchThreshold = searchThreshold;
        return this;
    }

    public FeatureHubConfiguration setSearchMode(int searchMode) {
        this.searchMode = searchMode;
        return this;
    }
    
}
