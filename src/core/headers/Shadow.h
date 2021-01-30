#pragma once

#include <Common.h>

/*
void SilhouetteDetection(const glm::vec4 lightPos,
                        Vertex* incomingVerts, u32 incVertCount,
                        u32* adjacency, u32 adjacencyCount,
                        std::vector<u32>& silhouetteEdges,std::vector<u32>& frontCaps,
                        std::vector<u32>& backCaps) {

    glm::vec3 worldPos[6];
    glm::vec3 e[6];
    glm::vec3 normal;
    glm::vec3 lightDir;

    for(int i = 0; i < adjacencyCount ; i+= 6) {

        worldPos[0] = incomingVerts[adjacency[i + 0]].p;
        worldPos[2] = incomingVerts[adjacency[i + 2]].p;
        worldPos[4] = incomingVerts[adjacency[i + 4]].p;

        e[0] = worldPos[2] - worldPos[0];
        e[1] = worldPos[4] - worldPos[0];

        normal = glm::cross(e[0],e[1]);
        lightDir = glm::vec3(lightPos) - worldPos[0];

        if( glm::dot(normal,lightDir) > 0 ) {

            frontCaps.push_back(adjacency[i + 0]);
            frontCaps.push_back(adjacency[i + 2]);
            frontCaps.push_back(adjacency[i + 4]);

            backCaps.push_back(adjacency[i + 0]);
            backCaps.push_back(adjacency[i + 4]);
            backCaps.push_back(adjacency[i + 2]);

            worldPos[1] = incomingVerts[adjacency[i + 1]].p;
            worldPos[3] = incomingVerts[adjacency[i + 3]].p;
            worldPos[5] = incomingVerts[adjacency[i + 5]].p;
            e[2] = worldPos[1] - worldPos[0];
            e[3] = worldPos[3] - worldPos[2];
            e[4] = worldPos[4] - worldPos[2];
            e[5] = worldPos[5] - worldPos[0];

            normal = glm::cross(e[2],e[0]);
            if( glm::dot(normal,lightDir) <= 0 ) {
                silhouetteEdges.push_back(adjacency[i + 0]);
                silhouetteEdges.push_back(adjacency[i + 2]);
            }

            normal = glm::cross(e[3],e[4]);
            lightDir = glm::vec3(lightPos) - worldPos[2];

            if( glm::dot(normal,lightDir) <= 0 ) {
                silhouetteEdges.push_back(adjacency[i + 2]);
                silhouetteEdges.push_back(adjacency[i + 4]);
            }

            normal = glm::cross(e[1],e[5]);
            lightDir = glm::vec3(lightPos) - worldPos[4];
            if( glm::dot(normal,lightDir) <= 0 ) {
                silhouetteEdges.push_back(adjacency[i + 4]);
                silhouetteEdges.push_back(adjacency[i + 0]);
            }
        }
    }
}
struct hash_pair {
   template <class T1, class T2>
   size_t operator()(const std::pair<T1, T2>& pair) const{
      auto hash1 = std::hash<T1>{}(pair.first);
      auto hash2 = std::hash<T2>{}(pair.second);
      return hash1 ^ hash2;
   }
};

void FindAdjecencies(const std::vector<unsigned int>& indices , std::vector<unsigned int>& adjacentindices) {
    
    adjacentindices.reserve(indices.size() * 2);
    std::unordered_map<std::pair<unsigned int , unsigned int>,unsigned int , hash_pair> map;
    std::pair<unsigned int, unsigned int>pair;

    for(int i = 0 ; i < indices.size() / 3 ; i++) {
        pair.first = indices[i * 3 + 0];
        pair.second = indices[i * 3 + 1];
        map[pair] = indices[i * 3 + 2];

        pair.first = indices[i * 3 + 1];
        pair.second = indices[i * 3 + 2];
        map[pair] = indices[i * 3 + 0];

        pair.first = indices[i * 3 + 2];
        pair.second = indices[i * 3 + 0];
        map[pair] = indices[i * 3 + 1];
    }

    for(int i = 0; i < indices.size() / 3 ; i++) {
        pair.first = indices[i * 3 + 1];
        pair.second = indices[i * 3 + 0];

        auto a = map.find(pair);
        if(a != map.end()) {
            adjacentindices.push_back(pair.second);
            adjacentindices.push_back(map[pair]);
        }
        else {
            std::cout << "no adjacency" << std::endl;
            adjacentindices.clear();
            return;
        }

        pair.first = indices[i * 3 + 2];
        pair.second = indices[i * 3 + 1];

        a = map.find(pair);
        if(a != map.end()) {
            adjacentindices.push_back(pair.second);
            adjacentindices.push_back(map[pair]);
        }
        else {
            std::cout << "no adjacency" << std::endl;
            adjacentindices.clear();
            return;
        }

        pair.first = indices[i * 3 + 0];
        pair.second = indices[i * 3 + 2];

        a = map.find(pair);
        if(a != map.end()) {
            adjacentindices.push_back(pair.second);
            adjacentindices.push_back(map[pair]);
        }
        else {
            std::cout << "no adjacency" << std::endl;
            adjacentindices.clear();
            return;
        }
    }
}
void MakeShadow(Vertex* verts , u32 count , u32* indices , u32 indCount, Vertex*& outVert , u32*& outInd , u32& outVCount, u32& outICount) {
    glm::vec3 p;
    std::vector<glm::vec3> outV;
    for(int i = 0; i < count ; i++) {

        p = verts[i].p;

        bool s = false;
        for(int k = 0; k < outV.size() ; k++ ) {
            if(p == outV[k] ) {
                s = true;
                break;
            }
        }

        if(!s) {
            outV.push_back(p);
        }
    }

    std::vector<u32> indexVector;
    for(int i = 0; i < indCount ;i++) {

        p = verts[indices[i]].p;

        for(int k = 0; k < outV.size() ; k++) {
            if(p == outV[k]) {
                indexVector.push_back(k);
                break;
            }
        }
    }
    auto s = indexVector.size();

    std::vector<u32> outI;
    FindAdjecencies(indexVector , outI );
    if(outI.size() == 0) {
        outVert = nullptr;
        outInd = nullptr;
        outVCount = 0;
        outICount = 0;
        return;
    }

    outVert = (Vertex*)malloc( outV.size() * sizeof(Vertex) );
    outInd = (u32*)malloc( outI.size() * sizeof(u32) );

    for(int i = 0; i < outV.size() ; i++) {
        outVert[i].p = outV[i];
        outVert[i].u = 0;
        outVert[i].v = 0;
    }
    for(int i = 0; i < outI.size() ; i++) {
        outInd[i] = outI[i];
    }
    outVCount = outV.size();
    outICount = outI.size();
}
*/