// Gmsh - Copyright (C) 1997-2017 C. Geuzaine, J.-F. Remacle
//
// See the LICENSE.txt file for license information. Please report all
// bugs and problems to the public mailing list <gmsh@onelab.info>.

#include "Mesh/GMSH/BackgroundMeshManager.h"
#include "Mesh/GMSH/BGMBase.h"
#include "Mesh/GMSH/BackgroundMesh2D.h"
#include "Mesh/GMSH/GEntity.h"
#include "Mesh/GMSH/GFace.h"
//#include "Mesh/GMSH/GRegion.h"
//#include "Mesh/GMSH/BackgroundMesh3D.h"

map<GEntity*,BGMBase*> BGMManager::data = map<GEntity*,BGMBase*>();
BGMBase* BGMManager::latest2Dbgm = NULL;
bool BGMManager::use_cross_field = true;

void BGMManager::set_use_cross_field(bool b)
{
  if (b && (BGMManager::use_cross_field==false)){// need to change...
    data.clear();
  }
  BGMManager::use_cross_field = b;
}

/*
BGMBase* BGMManager::get(GRegion* gf)
{
  map<GEntity*,BGMBase*>::iterator itfind = data.find(gf);
  if (itfind!=data.end()){
    return itfind->second;
  }

  BGMBase *bgm;
  if (use_cross_field){
    bgm = new frameFieldBackgroundMesh3D(gf);
  }
  else{
    bgm = new backgroundMesh3D(gf);
  }
  data.insert(make_pair(gf,bgm));
  return bgm;
}
 */ 

BGMBase* BGMManager::get(GFace* gf)
{
  map<GEntity*,BGMBase*>::iterator itfind = data.find(gf);
  if (itfind!=data.end()){
    latest2Dbgm = itfind->second;
    return itfind->second;
  }

  BGMBase *bgm;
  if (use_cross_field)
    bgm = new frameFieldBackgroundMesh2D(gf);
  else
    bgm = new backgroundMesh2D(gf);
  data.insert(make_pair(gf,bgm));
  latest2Dbgm = bgm;
  return bgm;
}

BGMBase* BGMManager::current2D()
{
  return latest2Dbgm;
};
