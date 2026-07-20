#pragma once

interface IGUIVisitor;

interface IGUIVisitable {
public:
    virtual void AcceptGUIVisitor(IGUIVisitor& v) = 0;
    virtual ~IGUIVisitable() = default;
};
