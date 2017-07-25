#include <QtWidgets>

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

#include <Eigen/Sparse>
#include <Eigen/Dense>

#include "ConsoleMessageManager.h"
#include "OpenGLCamera.h"
#include "Vertex.h"
#include "TextConfigLoader.h"
#include "kdTreeSolution.h"
#include "OcTreeFieldSolution.h"