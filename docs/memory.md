# Memory Index

- [User Profile](user_profile.md) — ELTE thesis student, Cone Step Mapping, C++/OpenGL, communicates in Hungarian
- [Project State](project_state.md) — teljes projekt állapot: forrásfa, osztályhierarchia, MyApp szétbontás, GUI struktúra, debug rendszer, vertex típusok, OBJ betöltés gotchák
- [Project Conventions](project_conventions.md) — coding style, design patterns, Vertex vs VertexMergedNorm, OBJ texture loading, interface kulcsszó, ShaderManager reload
- [Workflow: temp/ staging](feedback_workflow.md) — all generated code goes to temp/ for review, never directly to src/
- [Performance Priority](feedback_performance.md) — runtime is always primary; compile time is never a concern
- [Phase 3 Reminder](project_phase3_reminder.md) — uncomment ConemapGenerator include in RayMarchedModel.cpp after Phase 3
- [AxesRenderer geometry](project_axes_renderer.md) — nincs VAO/VBO, a tengelyek geometriája a shaderbe van égetve
- [UML field notation](project_uml_notation.md) — `(+,+)` = private mező + public getter/setter; `(+,!)` = csak getter; nem UML standard, projekt-konvenció
- [Deferred: cube-sphere heightmap](project_deferred_cube_sphere.md) — alpha cutout előfeltétel; matematika kész, Python script halasztva