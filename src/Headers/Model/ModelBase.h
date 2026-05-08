#pragma once

#include <string>

// GLM
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

// GLEW
#include <GL/glew.h>

#include "../Transformation/Transformation.h"
#include "../../Interfaces/IDrawable.h"
#include "../../Utils/Log.h"
#include "../Types.h"

class ModelBase : public IDrawable {
protected:
	inline static int count = 0;
	GLuint m_programID;
	std::vector<Transformation*> m_transforms{};
	glm::mat4 m_transform{ glm::identity<glm::mat4>() };
	bool m_show;
	int m_drawMode;
	std::string m_name;
	bool m_transformDirty = true;
	bool m_applyTransforms = true;
	bool m_deleteMarker = false;

	char m_objNameBuffer[64] = "";

public:
	ModelBase(const ModelBaseParams& params) {
		m_programID = params.shaderPrograms.programID;
		m_show = params.show;
		m_drawMode = params.drawMode;
		AddTransform();
		if (std::strcmp("", params.name) == 0) {
			m_name = "Model " + std::to_string(ModelBase::count);
		}
		else {
			m_name = params.name;
		}
		strcpy_s(m_objNameBuffer, m_name.c_str());

		++ModelBase::count;
	}

	void MarkForDeletion() {
		m_deleteMarker = true;
	}
	bool MarkedForDeletion() {
		return m_deleteMarker;
	}

	std::string GetName() const {
		return m_name;
	}
	void SetName() {
		if (std::strcmp("", m_objNameBuffer) == 0) {
			LOG_ERROR("Invalid object name");
			return;
		}
		m_name = m_objNameBuffer;
	}
	void SetName(const char* name) {
		strcpy_s(m_objNameBuffer, name);
		SetName();
	}

	GLuint GetProgramID() const {
		return m_programID;
	}
	void SetProgramID(GLuint programID) {
		m_programID = programID;
	}

	Transformation GetTransform(int index) const {
		if (index >= m_transforms.size()) {
			LOG_ERROR("Model::GetTransform index out of range %d", index);
			return Transformation();
		}
		return *m_transforms[index];
	}
	virtual glm::mat4 GetTransform() {
		// check if any of the transformations is changed
		bool isDirty = false;
		for (auto t : m_transforms) {
			if (t->IsDirty()) {
				isDirty = true;
				t->Clean();
			}
		}
		// calculate transformation if changed
		if (isDirty || m_transformDirty) {
			m_transformDirty = false;
			glm::mat4 acc = glm::identity<glm::mat4>();
			for (int i = m_transforms.size() - 1; i >= 0; --i) {
				acc *= m_transforms[i]->Get();
			}
			m_transform = acc;
		}
		return m_transform;
	}
	std::vector<Transformation*> GetTransforms() const {
		return m_transforms;
	}
	void SetTransforms(std::vector<Transformation*> transforms) {
		m_transforms.clear();
		for (Transformation* t : transforms) {
			m_transforms.push_back(new Transformation(t->Get()));
		}
		m_transformDirty = true;
	}
	void AddTransform(glm::mat4 transform) {
		m_transforms.push_back(new Transformation(transform));
	}
	void AddTransform() {
		m_transforms.push_back(new Transformation());
	}
	void DelTransform(int index) {
		if (index >= 0 && index < m_transforms.size()) {
			delete(m_transforms[index]);
			m_transforms.erase(m_transforms.begin() + index);
			m_transformDirty = true;
		}
		if (m_transforms.size() < 1) {
			AddTransform();
		}
	}
	void DelTransforms() {
		for (auto t : m_transforms) {
			delete(t);
		}
	}

	void SetShow(bool show) {
		m_show = show;
	}
	bool GetShow() const {
		return m_show;
	}

	void SetDrawMode(int drawMode) {
		m_drawMode = drawMode;
	}
	int GetDrawMode() const {
		return m_drawMode;
	}

	virtual void SetApplyTransforms(bool apply) {
		m_applyTransforms = apply;
	}
	bool GetApplyTransforms() const {
		return m_applyTransforms;
	}

	// iterator
	std::vector<Transformation*>::iterator begin() {
		return m_transforms.begin();
	}
	std::vector<Transformation*>::iterator end() {
		return m_transforms.end();
	}
	std::vector<Transformation*>::const_iterator begin() const {
		return m_transforms.begin();
	}
	std::vector<Transformation*>::const_iterator end() const {
		return m_transforms.end();
	}

	~ModelBase() {
		DelTransforms();
	}

};