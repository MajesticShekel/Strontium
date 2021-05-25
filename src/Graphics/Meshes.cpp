#include "Graphics/Meshes.h"

// Project includes.
#include "Core/Logs.h"

namespace SciRenderer
{
  // Constructors
  Mesh::Mesh()
    : loaded(false)
    , modelMatrix(glm::mat4(1.0f))
    , hasUVs(false)
    , vArray(nullptr)
  { }

  Mesh::Mesh(const std::vector<Vertex> &vertices, const std::vector<GLuint> &indices)
    : loaded(true)
    , data(vertices)
    , indices(indices)
    , modelMatrix(glm::mat4(1.0f))
    , hasUVs(false)
    , vArray(nullptr)
  { }

  Mesh::~Mesh()
  { }

  // Loading function for .obj files.
  void
  Mesh::loadOBJFile(const std::string &filepath, bool computeTBN)
  {
    Logger* logs = Logger::getInstance();

    tinyobj::ObjReader objReader;
    tinyobj::ObjReaderConfig readerConfig;
    readerConfig.mtl_search_path = "";

    if (!objReader.ParseFromFile(filepath.c_str(), readerConfig))
    {
      if (!objReader.Error().empty())
      {
        std::cerr << "Error loading file (" << filepath << "): "
                  << objReader.Error() << std::endl;
        logs->logMessage(LogMessage("Error loading file (" + filepath + "): "
                                    + objReader.Error(), true, false, true));
      }
    }

    tinyobj::attrib_t attrib             = objReader.GetAttrib();
    std::vector<tinyobj::shape_t> shapes = objReader.GetShapes();
    this->materials                      = objReader.GetMaterials();

    // Get the vertex components.
    for (unsigned i = 0; i < attrib.vertices.size(); i+=3)
    {
      Vertex temp;
      temp.position = glm::vec4(attrib.vertices[i],
                                attrib.vertices[i + 1],
                                attrib.vertices[i + 2],
                                1.0f);
      this->data.push_back(temp);
    }

    // Get the indices.
    unsigned vertexOffset = 0;
    for (unsigned i = 0; i < shapes.size(); i++)
    {
      for (unsigned j = 0; j < shapes[i].mesh.indices.size(); j++)
      {
        this->indices.push_back(shapes[i].mesh.indices[j].vertex_index
                                + vertexOffset);
      }
      vertexOffset += shapes[i].mesh.indices.size();
    }

    // Compute the normal vectors if the obj has none.
    vertexOffset = 0;
    if (attrib.normals.size() == 0)
      this->computeNormals();
    else if (attrib.normals.size() > 0)
    {
      glm::vec3 temp;
      for (unsigned i = 0; i < shapes.size(); i++)
      {
        for (unsigned j = 0; j < shapes[i].mesh.indices.size(); j++)
        {
          temp.x = attrib.normals[3 * (shapes[i].mesh.indices[j].normal_index)];
          temp.y = attrib.normals[3 * (shapes[i].mesh.indices[j].normal_index) + 1];
          temp.z = attrib.normals[3 * (shapes[i].mesh.indices[j].normal_index) + 2];
          this->data[shapes[i].mesh.indices[j].vertex_index].normal = temp;
        }
      }
    }

    // Get the UVs.
    if (attrib.texcoords.size() > 0)
    {
      glm::vec2 temp;
      for (unsigned i = 0; i < shapes.size(); i++)
      {
        for (unsigned j = 0; j < shapes[i].mesh.indices.size(); j++)
        {
          temp.x = attrib.texcoords[2 * (shapes[i].mesh.indices[j].texcoord_index)];
          temp.y = attrib.texcoords[2 * (shapes[i].mesh.indices[j].texcoord_index) + 1];
          this->data[shapes[i].mesh.indices[j].vertex_index].uv = temp;
        }
      }
      this->hasUVs = true;
    }

    // Get the colours for each vertex if they exist.
    if (attrib.colors.size() > 0)
    {
      glm::vec3 temp;
      for (unsigned i = 0; i < shapes.size(); i++)
      {
        for (unsigned j = 0; j < shapes[i].mesh.indices.size(); j++)
        {
          temp.x = attrib.colors[2 * (shapes[i].mesh.indices[j].vertex_index)];
          temp.y = attrib.colors[2 * (shapes[i].mesh.indices[j].vertex_index) + 1];
          temp.z = attrib.colors[2 * (shapes[i].mesh.indices[j].vertex_index) + 2];
          this->data[shapes[i].mesh.indices[j].vertex_index].colour = temp;
        }
      }
    }
    else
    {
      for (unsigned i = 0; i < this->data.size(); i++)
        this->data[i].colour = glm::vec3(1.0f, 1.0f, 1.0f);
    }

    if (computeTBN)
      this->computeTBN();

    this->filepath = filepath;
    this->loaded = true;
  }

  void
  Mesh::generateVAO()
  {
    if (!this->isLoaded())
      return;
    this->vArray = createShared<VertexArray>(&(this->data[0]),
                                             this->data.size() * sizeof(Vertex),
                                             BufferType::Dynamic);
    this->vArray->addIndexBuffer(&(this->indices[0]), this->indices.size(),
                                 BufferType::Dynamic);
  }

  // Generate a vertex array object associated with this mesh.
  void
  Mesh::generateVAO(Shared<Shader> program)
  {
    if (!this->isLoaded())
      return;
    this->vArray = createShared<VertexArray>(&(this->data[0]),
                                             this->data.size() * sizeof(Vertex),
                                             BufferType::Dynamic);
    this->vArray->addIndexBuffer(&(this->indices[0]), this->indices.size(), BufferType::Dynamic);

  	program->addAtribute("vPosition", VEC4, GL_FALSE, sizeof(Vertex), 0);
  	program->addAtribute("vNormal", VEC3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, normal));
  	program->addAtribute("vColour", VEC3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, colour));
    program->addAtribute("vTexCoord", VEC2, GL_FALSE, sizeof(Vertex), offsetof(Vertex, uv));
    program->addAtribute("vTangent", VEC3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, tangent));
    program->addAtribute("vBitangent", VEC3, GL_FALSE, sizeof(Vertex), offsetof(Vertex, bitangent));
  }

  // Helper function to bulk compute normals.
  void
  Mesh::computeNormals()
  {
    // Temporary variables to make life easier.
    glm::vec3              a, b, c, edgeOne, edgeTwo, cross;
    std::vector<glm::vec3> vertexNormals;
    std::vector<unsigned>  count;

    for (unsigned i = 0; i < this->data.size(); i++)
    {
      vertexNormals.emplace_back(glm::vec3(0.0f, 0.0f, 0.0f));
      count.push_back(0);
    }

    // Compute the normals of each face.
    for (unsigned i = 0; i < this->indices.size(); i+=3)
    {
      a = this->data[this->indices[i]].position;
      b = this->data[this->indices[i + 1]].position;
      c = this->data[this->indices[i + 2]].position;
      count[this->indices[i]] += 1;
      count[this->indices[i + 1]] += 1;
      count[this->indices[i + 2]] += 1;

      // Compute triangle face normals.
      edgeOne = b - a;
      edgeTwo = c - a;
      cross = glm::normalize(glm::cross(edgeOne, edgeTwo));

      // Add the face normal to the vertex normal array.
      vertexNormals[this->indices[i]]     += cross;
      vertexNormals[this->indices[i + 1]] += cross;
      vertexNormals[this->indices[i + 2]] += cross;
    }

    // Loop over the vertex normal array and normalize the vectors.
    for (unsigned i = 0; i < vertexNormals.size(); i++)
    {
      vertexNormals[i][0] /= count[i];
      vertexNormals[i][1] /= count[i];
      vertexNormals[i][2] /= count[i];
      vertexNormals[i] = glm::normalize(vertexNormals[i]);
      this->data[i].normal = vertexNormals[i];
    }
  }

  // Helper function to bulk compute tangents and bitangents.
  void
  Mesh::computeTBN()
  {
    glm::vec3 tangent, bitangent, edgeOne, edgeTwo;
    glm::vec2 duvOne, duvTwo;
    Vertex a, b, c;
    float det;

    for (unsigned i = 0; i < this->indices.size(); i+=3)
    {
      a = this->data[this->indices[i]];
      b = this->data[this->indices[i + 1]];
      c = this->data[this->indices[i + 2]];

      edgeOne = b.position - a.position;
      edgeTwo = c.position - a.position;

      duvOne = b.uv - a.uv;
      duvTwo = c.uv - a.uv;

      det = 1.0f / (duvOne.x * duvTwo.y - duvTwo.x * duvOne.y);

      tangent.x = det * (duvTwo.y * edgeOne.x - duvOne.y * edgeTwo.x);
      tangent.y = det * (duvTwo.y * edgeOne.y - duvOne.y * edgeTwo.y);
      tangent.z = det * (duvTwo.y * edgeOne.z - duvOne.y * edgeTwo.z);

      bitangent.x = det * (-duvTwo.x * edgeOne.x + duvOne.x * edgeTwo.x);
      bitangent.y = det * (-duvTwo.x * edgeOne.y + duvOne.x * edgeTwo.y);
      bitangent.z = det * (-duvTwo.x * edgeOne.z + duvOne.x * edgeTwo.z);

      this->data[this->indices[i]].tangent = tangent;
      this->data[this->indices[i + 1]].tangent= tangent;
      this->data[this->indices[i + 2]].tangent= tangent;

      this->data[this->indices[i]].bitangent += bitangent;
      this->data[this->indices[i + 1]].bitangent += bitangent;
      this->data[this->indices[i + 2]].bitangent += bitangent;
    }
  }

  void
  Mesh::setColour(const glm::vec3 &colour)
  {
    for (unsigned i = 0; i < this->data.size(); i++)
      this->data[i].colour = colour;
  }

  // Debugging helper function to dump mesh data to the console.
  void
  Mesh::dumpMeshData()
  {
    printf("Dumping vertex coordinates (%ld):\n", this->data.size());
    for (unsigned i = 0; i < this->data.size(); i++)
    {
      printf("V%d: (%f, %f, %f, %f)\n", i, this->data[i].position[0],
             this->data[i].position[1], this->data[i].position[2],
             this->data[i].position[3]);
    }
    printf("\nDumping vertex normals (%ld):\n", this->data.size());
    for (unsigned i = 0; i < this->data.size(); i++)
    {
      printf("N%d: (%f, %f, %f)\n", i, this->data[i].normal[0], this->data[i].normal[1],
             this->data[i].normal[2]);
    }
    printf("\nDumping indices (%ld):\n", this->indices.size());
    for (unsigned i = 0; i < this->indices.size(); i+=3)
    {
      printf("I%d: (%d, %d, %d)\n", i, this->indices[i], this->indices[i + 1],
             this->indices[i + 2]);
    }
  }
}
