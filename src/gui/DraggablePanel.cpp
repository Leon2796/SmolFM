/*
    DraggablePanel implementation.
*/

#include "DraggablePanel.h"

namespace gui
{

namespace
{
    juce::String makeDefaultPositionKey (const juce::String& boxId)
    {
        return "pos_" + boxId;   // e.g. pos_carrier, pos_modulator, ...
    }

    void writeBoxPosition (juce::PropertiesFile& props,
                           const DraggableComponent& box)
    {
        const juce::Rectangle<int> r = box.getBounds();
        // Compact storage: one string per box, no XML noise.
        props.setValue (makeDefaultPositionKey (box.getBoxId()),
                        juce::String (r.getX()) + "," + juce::String (r.getY()));
    }

    juce::String makeWiringKey()
    {
        return "connections_json";
    }

    bool readBoxPosition (juce::PropertiesFile& props,
                          const juce::String& boxId,
                          juce::Point<int>& outPosition)
    {
        const juce::String s = props.getValue (makeDefaultPositionKey (boxId));
        if (s.isEmpty())
            return false;

        const int comma = s.indexOfChar (',');
        if (comma < 0)
            return false;

        outPosition = { s.substring (0, comma).getIntValue(),
                        s.substring (comma + 1).getIntValue() };
        return true;
    }
}

//==============================================================================
DraggablePanel::DraggablePanel()
{
    // The panel itself is transparent and only carries the boxes.
    setOpaque (false);
}

DraggablePanel::~DraggablePanel()
{
    saveLayout();
}

DraggableComponent& DraggablePanel::addComponent (std::unique_ptr<DraggableComponent> box)
{
    jassert (box != nullptr);
    auto* raw = box.get();
    boxes.add (std::move (box));
    addAndMakeVisible (*raw);

    raw->onCloseRequested = [this] (DraggableComponent& b) { removeNode (b); };
    return *raw;
}

//==============================================================================
DraggableComponent* DraggablePanel::addNodeOfType (const juce::String& baseId,
                                                   juce::AudioProcessorValueTreeState& apvts,
                                                   std::function<std::unique_ptr<juce::Component> (const juce::String& instanceId,
                                                                                                   juce::AudioProcessorValueTreeState&)> makeContent)
{
    // Find the lowest free instance index for this type (or the only allowed
    // id for single-instance types like note/adsr).
    const smolfm::NodeType type = smolfm::GraphNodeRegistry::typeOf (baseId);
    const int max = smolfm::GraphNodeRegistry::maxInstancesOf (type);
    if (max <= 0)
        return nullptr;

    int index = -1;
    juce::String instanceId;

    if (max == 1)
    {
        instanceId = baseId;
        if (findBox (instanceId) != nullptr)
            return nullptr;      // only one allowed
        index = 0;
    }
    else
    {
        for (int i = 0; i < max; ++i)
        {
            const juce::String candidate = smolfm::GraphNodeRegistry::makeInstanceId (baseId, i);
            if (findBox (candidate) == nullptr)
            {
                index = i;
                instanceId = candidate;
                break;
            }
        }
        if (index < 0)
            return nullptr;      // budget exhausted
    }

    const smolfm::NodeSpec* spec = smolfm::GraphNodeRegistry::findSpec (baseId);
    if (spec == nullptr || makeContent == nullptr)
        return nullptr;

    auto content = makeContent (instanceId, apvts);
    if (content == nullptr)
        return nullptr;

    juce::String title = spec->title;
    if (max > 1)
        title += " " + juce::String (index + 1);

    auto box = std::make_unique<DraggableComponent> (instanceId, title, std::move (content));
    auto* raw = &addComponent (std::move (box));

    // Attach the pins declared by this node's spec.
    for (int i = 0; i < spec->inputPortIds.size(); ++i)
    {
        const smolfm::PortType t = i < static_cast<int> (spec->inputTypes.size())
                                       ? spec->inputTypes[static_cast<size_t> (i)]
                                       : smolfm::PortType::signal;
        raw->addInputPin (spec->inputPortIds[i], t);
    }

    if (spec->outputPortId.isNotEmpty())
        raw->addOutputPin (spec->outputPortId, spec->outputType);

    // Park the new box beside the previous ones so they don't stack exactly.
    raw->setTopLeftPosition ({ 16 + 24 * (boxes.size() - 1), 16 + 24 * (boxes.size() - 1) });

    if (onNodeSetChanged)
        onNodeSetChanged();

    repaint();
    return raw;
}

void DraggablePanel::removeNode (DraggableComponent& box)
{
    // Remove every wire that touches this box, then the box itself.
    removeAllConnectionsForBox (box.getBoxId());

    boxes.removeObject (&box);

    if (onNodeSetChanged)
        onNodeSetChanged();

    repaint();
}

//==============================================================================
DraggableComponent* DraggablePanel::findBox (const juce::String& boxId) noexcept
{
    for (auto* b : boxes)
        if (b->getBoxId() == boxId)
            return b;
    return nullptr;
}

juce::StringArray DraggablePanel::getBoxIds() const
{
    juce::StringArray ids;
    for (const auto* b : boxes)
        ids.add (b->getBoxId());
    return ids;
}

juce::Rectangle<int> DraggablePanel::getBoxBounds (const juce::String& boxId) const
{
    for (const auto* b : boxes)
        if (b->getBoxId() == boxId)
            return b->getBounds();

    return {};
}

void DraggablePanel::setBoxPosition (const juce::String& boxId, juce::Point<int> pos)
{
    if (auto* box = findBox (boxId))
    {
        pos.x = juce::jlimit (0, juce::jmax (0, getWidth()  - box->getWidth()),  pos.x);
        pos.y = juce::jlimit (0, juce::jmax (0, getHeight() - box->getHeight()), pos.y);
        box->setTopLeftPosition (pos);
        repaint();
    }
}

int DraggablePanel::countBoxesOfType (const juce::String& baseId) const
{
    int n = 0;
    for (const auto* b : boxes)
        if (smolfm::GraphNodeRegistry::baseIdOf (b->getBoxId()) == baseId)
            ++n;
    return n;
}

bool DraggablePanel::hasBox (const juce::String& baseId) const
{
    return countBoxesOfType (baseId) > 0;
}

PinComponent* DraggablePanel::findInputPin (const juce::String& nodeId, const juce::String& portId) noexcept
{
    if (auto* box = findBox (nodeId))
        for (int i = 0; i < box->getNumInputPins(); ++i)
            if (box->getInputPin (i)->getPinId() == portId)
                return box->getInputPin (i);
    return nullptr;
}

PinComponent* DraggablePanel::findOutputPin (const juce::String& nodeId, const juce::String& portId) noexcept
{
    if (auto* box = findBox (nodeId))
        for (int i = 0; i < box->getNumOutputPins(); ++i)
            if (box->getOutputPin (i)->getPinId() == portId)
                return box->getOutputPin (i);
    return nullptr;
}

//==============================================================================
bool DraggablePanel::typesMatch (const smolfm::ConnectionPatch::Endpoint& from,
                                 const smolfm::ConnectionPatch::Endpoint& to)
{
    auto fromInfo = smolfm::GraphNodeRegistry::findPortInfo (from.nodeId, from.portId, true);
    auto toInfo   = smolfm::GraphNodeRegistry::findPortInfo (to.nodeId,   to.portId,   false);

    if (! fromInfo.first || ! toInfo.first)
        return false;

    return fromInfo.second == toInfo.second;
}

bool DraggablePanel::tryConnect (const smolfm::ConnectionPatch::Endpoint& from,
                                 const smolfm::ConnectionPatch::Endpoint& to)
{
    if (! typesMatch (from, to))
        return false;

    // One wire per input pin — remove any existing one first.
    removeConnectionTo (to);

    smolfm::ConnectionPatch::Connection conn { from, to };
    currentPatch.connections.push_back (conn);

    if (onConnectionPatchChanged)
        onConnectionPatchChanged (currentPatch);

    repaint();
    return true;
}

bool DraggablePanel::removeConnectionTo (const smolfm::ConnectionPatch::Endpoint& to)
{
    auto& cs = currentPatch.connections;
    for (size_t i = 0; i < cs.size(); ++i)
    {
        if (cs[i].to == to)
        {
            cs.erase (cs.begin() + static_cast<ptrdiff_t> (i));

            if (onConnectionPatchChanged)
                onConnectionPatchChanged (currentPatch);

            repaint();
            return true;
        }
    }
    return false;
}

bool DraggablePanel::isConnected (const smolfm::ConnectionPatch::Endpoint& to) const noexcept
{
    for (const auto& c : currentPatch.connections)
        if (c.to == to)
            return true;
    return false;
}

void DraggablePanel::removeAllConnectionsForBox (const juce::String& boxId)
{
    auto& cs = currentPatch.connections;

    for (size_t i = cs.size(); i-- > 0; )
    {
        if (cs[i].from.nodeId == boxId || cs[i].to.nodeId == boxId)
            cs.erase (cs.begin() + static_cast<ptrdiff_t> (i));
    }

    if (onConnectionPatchChanged)
        onConnectionPatchChanged (currentPatch);
}

void DraggablePanel::applyPatch (const smolfm::ConnectionPatch& patch)
{
    if (currentPatch == patch)
        return;

    currentPatch = patch;

    if (onConnectionPatchChanged)
        onConnectionPatchChanged (currentPatch);

    repaint();
}

//==============================================================================
void DraggablePanel::pinMouseDown (PinComponent& pin, const juce::MouseEvent& e)
{
    if (pin.isOutput())
    {
        draggingWire = true;
        draggedFrom = { pin.getBoxId(), pin.getPinId() };
        draggedToPosition = e.getEventRelativeTo (this).position;
        repaint();
        return;
    }

    // Clicking a connected input pin removes the wire.
    const smolfm::ConnectionPatch::Endpoint inputEp { pin.getBoxId(), pin.getPinId() };
    if (isConnected (inputEp))
    {
        // Right-drag starts a re-patching drag from the wire's output side;
        // the wire is only dropped for real on mouse-up without a valid target.
        if (e.mods.isRightButtonDown())
        {
            for (const auto& c : currentPatch.connections)
            {
                if (c.to == inputEp)
                {
                    draggingWire = true;
                    draggedFrom = c.from;
                    draggedToPosition = e.getEventRelativeTo (this).position;
                    repaint();
                    return;
                }
            }
        }

        removeConnectionTo (inputEp);
        return;
    }

    juce::ignoreUnused (e);
}

void DraggablePanel::pinMouseDrag (PinComponent& pin, const juce::MouseEvent& e)
{
    // pin.isOutput() would reject re-patch drags started from an INPUT pin.
    if (! draggingWire)
        return;

    draggedToPosition = e.getEventRelativeTo (this).position;
    repaint();
    juce::ignoreUnused (pin);
}

void DraggablePanel::pinMouseUp (PinComponent& pin, const juce::MouseEvent& e)
{
    if (! draggingWire)
        return;

    draggingWire = false;
    juce::ignoreUnused (pin);

    const juce::Point<float> panelPoint = e.getEventRelativeTo (this).position;
    const smolfm::ConnectionPatch::Endpoint from = draggedFrom;   // draggedFrom may come from an input pin's existing wire

    for (auto* box : boxes)
    {
        for (int i = 0; i < box->getNumInputPins(); ++i)
        {
            auto* cand = box->getInputPin (i);
            const auto centre = cand->getPinCentreInPanel();
            constexpr float hitRadius = 16.0f;
            if (centre.getDistanceFrom (panelPoint) <= hitRadius)
            {
                tryConnect (from, { cand->getBoxId(), cand->getPinId() });
                repaint();
                return;
            }
        }
    }

    repaint();
}

//==============================================================================
void DraggablePanel::paintOverChildren (juce::Graphics& g)
{
    // Subtle dark backdrop so the boxes stand out without looking busy.
    g.fillAll (juce::Colours::black.withAlpha (0.25f));

    auto drawWire = [&g] (juce::Point<float> from, juce::Point<float> to, smolfm::PortType type, float alpha)
    {
        const float dx = juce::jmax (40.0f, std::abs (to.x - from.x) * 0.5f);

        juce::Path path;
        path.startNewSubPath (from);
        path.cubicTo ({ from.x + dx, from.y },
                      { to.x - dx, to.y },
                      to);

        const auto colour = type == smolfm::PortType::frequency
                                ? juce::Colours::orange
                                : juce::Colours::cyan;

        g.setColour (colour.withAlpha (alpha));
        g.strokePath (path, juce::PathStrokeType (2.0f));
    };

    // Paint every established connection.
    for (const auto& c : currentPatch.connections)
    {
        auto* fromBox = findBox (c.from.nodeId);
        auto* toBox   = findBox (c.to.nodeId);
        if (fromBox == nullptr || toBox == nullptr)
            continue;

        juce::Point<float> p1, p2;
        for (int i = 0; i < fromBox->getNumOutputPins(); ++i)
        {
            auto* p = fromBox->getOutputPin (i);
            if (p->getPinId() == c.from.portId)
                p1 = fromBox->getPosition().toFloat() + p->getBounds().getCentre().toFloat();
        }
        for (int i = 0; i < toBox->getNumInputPins(); ++i)
        {
            auto* p = toBox->getInputPin (i);
            if (p->getPinId() == c.to.portId)
                p2 = toBox->getPosition().toFloat() + p->getBounds().getCentre().toFloat();
        }

        if (p1.getDistanceFrom ({}) < 1.0f || p2.getDistanceFrom ({}) < 1.0f)
            continue;

        auto fromInfo = smolfm::GraphNodeRegistry::findPortInfo (c.from.nodeId, c.from.portId, true);
        drawWire (p1, p2, fromInfo.second, 0.9f);
    }

    // Paint the in-flight drag wire on top.
    if (draggingWire)
    {
        if (auto* fromBox = findBox (draggedFrom.nodeId))
        {
            for (int i = 0; i < fromBox->getNumOutputPins(); ++i)
            {
                auto* p = fromBox->getOutputPin (i);
                if (p->getPinId() == draggedFrom.portId)
                {
                    const auto start = fromBox->getPosition().toFloat()
                                       + p->getBounds().getCentre().toFloat();

                    auto fromInfo = smolfm::GraphNodeRegistry::findPortInfo (draggedFrom.nodeId,
                                                                             draggedFrom.portId,
                                                                             true);
                    drawWire (start, draggedToPosition, fromInfo.second, 0.6f);
                    break;
                }
            }
        }
    }
}

void DraggablePanel::resized()
{
    // Layout is free-form; boxes are moved by the user, not by the panel.
    // Nothing to do here on purpose.
}

//==============================================================================
juce::PropertiesFile& DraggablePanel::getPropertiesFile()
{
    if (propertiesFile == nullptr)
    {
        juce::PropertiesFile::Options options;
        options.applicationName     = "SmolFM";
        options.folderName          = "SmolFM";
        options.filenameSuffix      = ".layout";
        options.osxLibrarySubFolder = "Application Support";
        options.commonToAllUsers    = false;
        options.ignoreCaseOfKeyNames = false;

        propertiesFile = std::make_unique<juce::PropertiesFile> (options);
    }

    return *propertiesFile;
}

void DraggablePanel::saveLayout()
{
    auto& props = getPropertiesFile();

    for (const DraggableComponent* box : boxes)
        writeBoxPosition (props, *box);

    props.setValue (makeWiringKey(), smolfm::ConnectionPatchIO::serializeToJson (currentPatch));
    props.saveIfNeeded();
}

void DraggablePanel::loadLayout()
{
    auto& props = getPropertiesFile();

    for (DraggableComponent* box : boxes)
    {
        juce::Point<int> pos;
        if (readBoxPosition (props, box->getBoxId(), pos))
        {
            pos.x = juce::jlimit (0, juce::jmax (0, getWidth()  - box->getWidth()),  pos.x);
            pos.y = juce::jlimit (0, juce::jmax (0, getHeight() - box->getHeight()), pos.y);
            box->setTopLeftPosition (pos);
        }
    }

    smolfm::ConnectionPatch loadedPatch;
    if (smolfm::ConnectionPatchIO::parseFromJson (props.getValue (makeWiringKey()), loadedPatch))
    {
        // Replace silently — the host applies the patch once at startup via
        // prepareToPlay / startNote.
        currentPatch = std::move (loadedPatch);
        if (onConnectionPatchChanged)
            onConnectionPatchChanged (currentPatch);
        repaint();
    }
}

} // namespace gui
