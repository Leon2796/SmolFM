/*
    DraggablePanel implementation.

    Layout and wiring live exclusively in the loaded .smolfm file.  Nothing is
    persisted to local properties anymore — the file is the single source of
    truth.
*/

#include "DraggablePanel.h"

#include <algorithm>
#include <set>
#include <vector>

namespace gui
{

//==============================================================================
DraggablePanel::DraggablePanel()
{
    // The panel itself is transparent and only carries the boxes.
    setOpaque (false);
}

DraggablePanel::~DraggablePanel()
{
}

DraggableComponent& DraggablePanel::addComponent (std::unique_ptr<DraggableComponent> box)
{
    jassert (box != nullptr);
    auto* raw = box.get();
    boxes.add (std::move (box));

    // Components start invisible by default.  Callers (toolbar or patch
    // loader) decide which ones should be shown.
    addChildComponent (*raw);

    raw->onCloseRequested = [this] (DraggableComponent& b) { removeNode (b); };
    return *raw;
}

//==============================================================================
DraggableComponent* DraggablePanel::addNodeOfType (const juce::String& baseId,
                                                   juce::AudioProcessorValueTreeState& apvts,
                                                   std::function<std::unique_ptr<juce::Component> (const juce::String& instanceId,
                                                                                                   juce::AudioProcessorValueTreeState&)> makeContent,
                                                   bool makeVisible)
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

    // Toolbar additions are immediately visible; patch-loaded nodes stay
    // hidden until updateVisibilityFromConnections() proves they're needed.
    raw->setVisible (makeVisible);

    // Optional bypass: signal/fq processors declare a "<base><N>Bypass"
    // template; this wires it to the title-bar bypass icon.  Every other type
    // gets an empty id back, so those nodes keep a plain title bar.
    raw->setBypassParameter (apvts, smolfm::GraphNodeRegistry::bypassParameterIdFor (instanceId));

    // Attach the pins declared by this node's spec.  Side hints (L/R
    // badges) come from the spec and are display-only.
    auto sideFor = [] (const std::vector<smolfm::ChannelSide>& sides, int index)
    {
        return index >= 0 && index < static_cast<int> (sides.size())
                   ? sides[static_cast<size_t> (index)]
                   : smolfm::ChannelSide::none;
    };

    for (int i = 0; i < spec->inputPortIds.size(); ++i)
    {
        const smolfm::PortType t = i < static_cast<int> (spec->inputTypes.size())
                                       ? spec->inputTypes[static_cast<size_t> (i)]
                                       : smolfm::PortType::signal;
        raw->addInputPin (spec->inputPortIds[i], t, sideFor (spec->inputChannelSides, i));
    }

    if (spec->outputPortId.isNotEmpty())
        raw->addOutputPin (spec->outputPortId, spec->outputType,
                           sideFor (spec->outputChannelSides, 0));

    // Extra outputs may carry their own type (note.vel is velocity while
    // note.out is frequency); missing entries fall back to outputType.
    for (int i = 0; i < spec->extraOutputPortIds.size(); ++i)
    {
        const smolfm::PortType t = i < static_cast<int> (spec->extraOutputTypes.size())
                                       ? spec->extraOutputTypes[static_cast<size_t> (i)]
                                       : spec->outputType;
        raw->addOutputPin (spec->extraOutputPortIds[i], t,
                           sideFor (spec->outputChannelSides, i + 1));
    }

    // Park the new box beside the previous ones so they don't stack exactly.
    raw->setTopLeftPosition ({ 16 + 24 * (boxes.size() - 1), 16 + 24 * (boxes.size() - 1) });

    // Grid mode: toolbar additions go straight into the unit grid.  Boxes
    // created hidden (patch loads) dock later via the reveal hook in
    // updateVisibilityFromConnections().
    if (gridMode && raw->isVisible())
        dockSingleBox (*raw);

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

bool DraggablePanel::removeLastNodeOfType (const juce::String& baseId)
{
    // Find the last box whose id starts with this base (e.g. "osc2" for "osc").
    for (int i = boxes.size() - 1; i >= 0; --i)
    {
        auto* box = boxes[i];
        if (box != nullptr && box->getBoxId().startsWith (baseId))
        {
            removeNode (*box);
            return true;
        }
    }
    return false;
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
                // Do NOT clamp against the panel size: the saved layout may extend
        // past the current (still small) canvas.  The editor grows the window
        // to fit right after loading.  Negative coordinates are still
        // nonsense, so only those are clamped away.
        box->setTopLeftPosition ({ juce::jmax (0, pos.x), juce::jmax (0, pos.y) });
        refreshCanvasSize();
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

void DraggablePanel::updateVisibilityFromConnections()
{
    // Collect the node ids that appear in at least one wire.
    std::set<juce::String> connectedNodeIds;
    for (const auto& c : currentPatch.connections)
    {
        connectedNodeIds.insert (c.from.nodeId);
        connectedNodeIds.insert (c.to.nodeId);
    }

    // Show only boxes that are part of the current wiring; hide the rest.
    // A box revealed while grid mode is on docks straight into the unit
    // grid, so it never reappears free-form between two cell lines.
    for (DraggableComponent* b : boxes)
    {
        const bool show = connectedNodeIds.count (b->getBoxId()) > 0;
        const bool wasVisible = b->isVisible();

        b->setVisible (show);

        if (show && gridMode && ! wasVisible)
            dockSingleBox (*b);
    }

    repaint();
}

//==============================================================================
void DraggablePanel::setGridMode (bool enabled)
{
    if (gridMode == enabled)
        return;

    gridMode = enabled;

    if (enabled)
        dockToGrid();

    // Disabling keeps the docked geometry (it stays valid free-form layout)
    // instead of restoring pre-dock sizes nobody recorded.
    repaint();
}

juce::Point<int> DraggablePanel::gridSnapped (juce::Point<int> position) const noexcept
{
    return { grid::snapTo (position.x), grid::snapTo (position.y) };
}

void DraggablePanel::dockSingleBox (DraggableComponent& box)
{
    // Obstacles: the boxes already placed in the grid (visible ones only).
    std::vector<juce::Rectangle<int>> occupied;

    for (DraggableComponent* b : boxes)
        if (b != &box && b->isVisible())
            occupied.push_back (b->getBounds());

    // The drop point decides: this probe walks over the whole canvas without
    // capping candidates at the panel edge.  The old keepInside cap was the
    // reason drops near the right/bottom edge sprang back to interior cells
    // (every candidate past the edge was skipped, so the ring walk returned
    // inward).  With a scrollable canvas a landing past the edge is anyway
    // reachable after a scroll — refreshCanvasSize() grows the canvas under
    // it and the surrounding Viewport shows its scrollbars when needed.
    box.setBounds (grid::dockedBounds (box.getBounds(), occupied));

    refreshCanvasSize();
    repaint();
}

void DraggablePanel::dockToGrid()
{
    // Reading order (top row first, then left column) gives a stable,
    // predictable dock result even when the loaded layout overlaps.
    std::vector<DraggableComponent*> visible;

    for (DraggableComponent* b : boxes)
        if (b->isVisible())
            visible.push_back (b);

    std::sort (visible.begin(), visible.end(),
               [] (const DraggableComponent* a, const DraggableComponent* b)
               {
                   if (a->getY() != b->getY())
                       return a->getY() < b->getY();
                   if (a->getX() != b->getX())
                       return a->getX() < b->getX();
                   return a->getBoxId() < b->getBoxId();
               });

    // Whole-canvas parking: docked boxes may extend past the current panel;
    // refreshCanvasSize() and fitWindowToContent() grow for them.
    std::vector<juce::Rectangle<int>> occupied;

    for (DraggableComponent* b : visible)
    {
        const juce::Rectangle<int> docked = grid::dockedBounds (b->getBounds(), occupied);
        b->setBounds (docked);
        occupied.push_back (docked);
    }

    refreshCanvasSize();
    repaint();
}

juce::Rectangle<int> DraggablePanel::getContentBounds() const noexcept
{
    juce::Rectangle<int> content;
    for (const auto* b : boxes)
        content = content.getUnion (b->getBounds());

    return content.expanded (16);   // padding so nothing hugs the edge
}

void DraggablePanel::refreshCanvasSize()
{
    // Single owner of the canvas-size rule; PluginEditor::resized() and the
    // dock paths above all end here.  The canvas at least fills the visible
    // area, so the surrounding Viewport's scrollbars appear exactly when the
    // canvas outgrows it — and disappear again once everything fits.
    // Grid mode adds one reserve cell beyond the content: a drop on the
    // outermost visible column/row must stay a legal dock target (the walk
    // in dockedBounds may look one cell past the landing corner).
    // ponytail: no shrink feedback beyond this rule — the canvas only ever
    // follows content + reserve; drag-at-edge autoscroll is the upgrade path.
    const juce::Component* parent = getParentComponent();
    const juce::Rectangle<int> content = getContentBounds();
    const int slack = gridMode ? grid::unit : 0;

    setSize (juce::jmax (parent != nullptr ? parent->getWidth()  : 0,
                         content.getRight()  + slack),
             juce::jmax (parent != nullptr ? parent->getHeight() : 0,
                         content.getBottom() + slack));
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
// Multi-select.  Double-click on a title bar toggles a box's mark, a
// right-button drag on empty canvas draws a rubber-band rectangle, a plain
// background click clears everything, and marked boxes move as one group.
//==============================================================================

void DraggablePanel::toggleBoxSelection (DraggableComponent& box)
{
    box.setSelected (! box.isSelected());
    repaint();
}

void DraggablePanel::clearSelection()
{
    bool changed = false;

    for (DraggableComponent* b : boxes)
        if (b->isVisible() && b->isSelected())
        {
            b->setSelected (false);
            changed = true;
        }

    if (changed)
        repaint();
}

void DraggablePanel::beginGroupDrag (DraggableComponent& origin)
{
    dragGroup.clear();
    dragGroupStart.clear();

    if (! origin.isSelected())
        return;   // dragging an unmarked box moves it alone

    // Dragged box first: moveGroupTo derives the shared delta from it and
    // endGroupDrag docks it separately before the peers.
    dragGroup.push_back (&origin);
    dragGroupStart.push_back (origin.getPosition());

    for (DraggableComponent* b : boxes)
        if (b != &origin && b->isVisible() && b->isSelected())
        {
            dragGroup.push_back (b);
            dragGroupStart.push_back (b->getPosition());
        }
}

void DraggablePanel::moveGroupTo (DraggableComponent& origin, juce::Point<int> originNewPos)
{
    if (dragGroup.empty() || dragGroup[0] != &origin)
        return;

    auto* parent = getParentComponent();
    if (parent == nullptr)
        return;

    const juce::Point<int> delta = originNewPos - dragGroupStart[0];
    const int slack = gridMode ? grid::unit : 0;

    for (int i = 1; i < static_cast<int> (dragGroup.size()); ++i)
    {
        DraggableComponent* peer = dragGroup[static_cast<size_t> (i)];

        // Same delta for the whole group; each peer clamps to the canvas
        // independently, so a wide peer never pushes the others when the
        // origin already sits at the edge.
        juce::Point<int> pos = dragGroupStart[static_cast<size_t> (i)] + delta;
        pos.x = juce::jlimit (0, juce::jmax (0, parent->getWidth()  - peer->getWidth()  + slack), pos.x);
        pos.y = juce::jlimit (0, juce::jmax (0, parent->getHeight() - peer->getHeight() + slack), pos.y);

        peer->setTopLeftPosition (pos);
    }
}

void DraggablePanel::endGroupDrag (DraggableComponent& origin)
{
    if (dragGroup.empty() || dragGroup[0] != &origin)
        return;

    // Grid mode: the dragged box already docked (dockSingleBox in mouseUp);
    // the peers walk from their dragged positions to nearby free cells in
    // capture order, so the group lands as close together as the occupied
    // cells allow.
    if (gridMode)
        for (int i = 1; i < static_cast<int> (dragGroup.size()); ++i)
            dockSingleBox (*dragGroup[static_cast<size_t> (i)]);

    dragGroup.clear();
    dragGroupStart.clear();
}

//==============================================================================
void DraggablePanel::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        // Rubber-band rectangle: press on empty canvas, drag, release.
        rubberBanding = true;
        rubberBandStart = e.getPosition();
        rubberBandNow = rubberBandStart;
        repaint();
        return;
    }

    // Plain click on the canvas background deselects everything.
    if (e.mods.isLeftButtonDown())
        clearSelection();
}

void DraggablePanel::mouseDrag (const juce::MouseEvent& e)
{
    if (! rubberBanding)
        return;

    rubberBandNow = e.getPosition();
    repaint();
}

void DraggablePanel::mouseUp (const juce::MouseEvent& e)
{
    if (! rubberBanding)
        return;

    rubberBanding = false;
    juce::ignoreUnused (e);

    const juce::Rectangle<int> rect (rubberBandStart, rubberBandNow);
    rubberBandStart = {};
    rubberBandNow = {};

    // A right-press without a real drag changes nothing (the canvas has no
    // context menu, so a stray click stays harmless).
    if (rect.getWidth() < 3 && rect.getHeight() < 3)
    {
        repaint();
        return;
    }

    // The rectangle replaces the previous selection: clear, then mark every
    // visible box it touches.
    clearSelection();

    for (DraggableComponent* b : boxes)
        if (b->isVisible() && b->getBounds().intersects (rect))
            b->setSelected (true);

    repaint();
}

//==============================================================================
void DraggablePanel::paint (juce::Graphics& g)
{
    // The unit grid draws under the boxes (and under the wires, which live
    // in paintOverChildren) so the canvas still reads as dark wiring board.
    if (! gridMode)
        return;

    g.setColour (juce::Colours::lightblue.withAlpha (0.10f));

    for (int x = 0; x < getWidth(); x += grid::unit)
        g.drawVerticalLine (x, 0.0f, static_cast<float> (getHeight()));

    for (int y = 0; y < getHeight(); y += grid::unit)
        g.drawHorizontalLine (y, 0.0f, static_cast<float> (getWidth()));
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

        // Colour legend: cyan = signal, orange = frequency, indianred =
        // velocity (connection bands of the per-note Anschlag send).
        const auto colour = type == smolfm::PortType::frequency
                                ? juce::Colours::orange
                                : (type == smolfm::PortType::velocity
                                       ? juce::Colours::indianred
                                       : juce::Colours::cyan);

        g.setColour (colour.withAlpha (alpha));
        g.strokePath (path, juce::PathStrokeType (2.0f));
    };

    // Small "L"/"R" tag at the wire midpoint, read from the source pin's
    // display-only side hint (splitter outputs, merger inputs).  Pure UI:
    // connect() and the audio path ignore this metadata.
    auto drawSideBadge = [&g] (juce::Point<float> from, juce::Point<float> to,
                               smolfm::ChannelSide side)
    {
        if (side == smolfm::ChannelSide::none)
            return;

        const auto mid = (from + to) * 0.5f;
        const auto badge = juce::Rectangle<float> (12.0f, 12.0f).withCentre (mid);

        g.setColour (side == smolfm::ChannelSide::left ? juce::Colours::cyan
                                                       : juce::Colours::pink);
        g.fillRoundedRectangle (badge, 3.0f);

        g.setColour (juce::Colours::black);
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText (side == smolfm::ChannelSide::left ? "L" : "R", badge,
                    juce::Justification::centred);
    };

    // Paint every established connection.
    for (const auto& c : currentPatch.connections)
    {
        auto* fromBox = findBox (c.from.nodeId);
        auto* toBox   = findBox (c.to.nodeId);
        if (fromBox == nullptr || toBox == nullptr)
            continue;

        juce::Point<float> p1, p2;
        smolfm::ChannelSide side = smolfm::ChannelSide::none;
        for (int i = 0; i < fromBox->getNumOutputPins(); ++i)
        {
            auto* p = fromBox->getOutputPin (i);
            if (p->getPinId() == c.from.portId)
            {
                p1 = fromBox->getPosition().toFloat() + p->getBounds().getCentre().toFloat();
                side = p->getChannelSide();
            }
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
        drawSideBadge (p1, p2, side);
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
                    drawSideBadge (start, draggedToPosition, p->getChannelSide());
                    break;
                }
            }
        }
    }

    // Rubber-band rectangle of an active right-button selection drag.
    if (rubberBanding)
    {
        const juce::Rectangle<float> rect { rubberBandStart.toFloat(), rubberBandNow.toFloat() };

        g.setColour (juce::Colours::yellow.withAlpha (0.12f));
        g.fillRect (rect);

        g.setColour (juce::Colours::yellow.withAlpha (0.8f));
        g.drawRoundedRectangle (rect, 2.0f, 1.0f);
    }
}

void DraggablePanel::resized()
{
    // Layout is free-form; boxes are moved by the user, not by the panel.
    // Nothing to do here on purpose.
}

//==============================================================================
void DraggablePanel::saveLayout()
{
    // No-op: layout and wiring are owned by the .smolfm file.
}

void DraggablePanel::loadLayout()
{
    // No-op: positions and wiring come from SmolFmFile::load only.
}

} // namespace gui
