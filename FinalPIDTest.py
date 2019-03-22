import pygame
import random
import math

# py-game variables
windowWidth = 1000
windowHeight = 800
pygame.init()
pygame.display.set_caption(str("Quad-copter balance"))
clock = pygame.time.Clock()
black = (0, 0, 0)
screen = pygame.display.set_mode((windowWidth, windowHeight))
screen.fill((255, 255, 255))
loopCount = 0
run = True
userangle = 0
# Fonts and other graphics
myFont = pygame.font.SysFont("Times New Roman", 18)
telemetries = (140, 170, 200, 230, 260, 290, 320, 350, 380, 410)
telem = 0
pidcorrect = 0


# Pretty self explanatory PID class
class PID:
    # declare PID variables
    pGain = 0
    iGain = 0
    dGain = 0
    target = 0
    reading = 0
    error = 0
    pTerm = 0
    iTerm = 0
    iLimit = 0.06
    errorSum = 0
    dTerm = 0
    errorSlope = 0
    lastError = 0
    correction = 0

    # For graphing, these store previous values
    errorlist = [0] * 196
    plist = [0] * 196
    ilist = [0] * 196
    dlist = [0] * 196
    correctlist = [0] * 196

    def __init__(self, pgain, igain, dgain):
        self.pGain = pgain
        self.iGain = igain
        self.dGain = dgain

    def getcorrection(self, targetvalue, currentreading):
        global loopCount
        self.target = targetvalue
        self.reading = currentreading
        self.error = self.target - self.reading
        self.pTerm = self.error * self.pGain
        self.errorSum += self.error
        self.iTerm = self.errorSum * self.iGain
        """
        if self.iTerm > self.iLimit:
            self.iTerm = self.iLimit
        elif self.iTerm < -self.iLimit:
            self.iTerm = -self.iLimit
        """
        if loopCount % 100 == 0:
            self.errorSum = 0
        self.errorSlope = (self.error - self.lastError)/0.1
        self.dTerm = self.errorSlope * self.dGain
        self.lastError = self.error
        self.correction = self.pTerm + self.iTerm + self.dTerm
        self.errorlist.append(self.error)
        del self.errorlist[0]
        self.plist.append(self.pTerm)
        del self.plist[0]
        self.ilist.append(self.iTerm)
        del self.ilist[0]
        self.dlist.append(self.dTerm)
        del self.dlist[0]
        self.correctlist.append(self.correction)
        del self.correctlist[0]
        return self.correction


# Quadcopter class, allowing you to make multiple Quads controlled by different PIDs
class Quadcopter:
    angularPos = 0
    angularVel = 0
    length = 0
    width = 0
    pivotX = 500
    pivotY = 300
    radius = 200

    def __init__(self, angPos, angVel, Len, Width, X, Y):
        self.angularPos = angPos
        self.angularVel = angVel
        self.length = Len
        self.width = Width
        self.pivotX = X
        self.radius = int(Len/2)
        self.pivotY = Y

    def draw(self):
        # Draws the quad-copter as a line
        # convert to radians
        angle = self.angularPos * 3.1415926536 / 180
        x1 = self.pivotX - (math.cos(angle) * self.radius)
        x2 = self.pivotX + (math.cos(angle) * self.radius)
        y1 = self.pivotY - (math.sin(angle) * self.radius)
        y2 = self.pivotY + (math.sin(angle) * self.radius)
        pygame.draw.line(screen, black, (x1, y1), (x2, y2), self.width)

    def disturbquad(self, wind, duration, time):
        # sways the quad-copter to simulate wind or imbalance
        wind = wind + random.randint(-1, 1) / 6
        wind /= 1000
        time *= 60
        if not duration == 0:
            duration *= 60
            if time < loopCount < time + duration:
                self.angularVel += wind
        else:
            self.angularVel += wind


# Creating Quadcopter and PID objects
quad = Quadcopter(80, 0, 400, 17, 500, 300)
pid = PID(0.2, 0.0003, 0.15)


def addTelemetry(caption, data):
    # this function displays the value of a variable
    global telem
    Label = myFont.render(caption + ":", 1, black)
    display = myFont.render(str(round(data, 4)), 1, black)
    screen.blit(Label, (10, telemetries[telem]))
    screen.blit(display, (len(caption)*8 + 20, telemetries[telem]))
    telem += 1


def handleloop():
    # this handles quitting and also updates the loop counters
    global telem, run, loopCount, mousePressed
    telem = 0
    loopCount += 1
    if loopCount == 100000:
        loopCount = 0
    screen.fill((255, 255, 255))
    for e in pygame.event.get():
        if e.type == pygame.QUIT:
            run = False


def mouseinput():
    # Gets the mouse input to allow the user to turn the quad
    global userangle
    mousepos = pygame.mouse.get_pos()
    opposite = mousepos[1] - quad.pivotY
    adjacent = mousepos[0] - quad.pivotX
    if not (mousepos[0] - quad.pivotX) == 0:
        userangle = math.atan(opposite / adjacent) * 180 / 3.1415926535
    if pygame.mouse.get_pressed()[0]:
        quad.angularPos = userangle
        quad.angularVel = 0


def graphpoints(inputlist, color, scale, ypos):
    # Takes in a list and displays it as a line graph
    actualx = 10
    for i in range(0, len(inputlist)-2):
        actualx += 5
        pygame.draw.line(screen, color, (actualx, int(scale * (inputlist[i]) + ypos)), (actualx + 5, int(scale * (inputlist[i+1]) + ypos)), 3)


while run:
    handleloop()
    pidcorrect = pid.getcorrection(0, quad.angularPos)
    # reduce the rate at which the motors update for realism
    if loopCount % 3 == 0:
        quad.angularVel += pidcorrect * 0.08
    # take mouse input
    mouseinput()
    # Increase Angular Position by Angular Velocity
    quad.angularPos += quad.angularVel
    # Graph the PID terms
    graphpoints(pid.errorlist, (255, 0, 0), 1, 100)
    graphpoints(pid.plist, (0, 255, 0), 5, 500)
    graphpoints(pid.ilist, (0, 0, 255), 15, 600)
    graphpoints(pid.dlist, (200, 190, 190), 5, 700)
    # Draw the Quad-copter
    quad.draw()
    # Display variables
    addTelemetry("Angular Position", quad.angularPos)
    addTelemetry("Angular Velocity", quad.angularVel)
    addTelemetry("error list", len(pid.errorlist))
    addTelemetry("PID correct", pidcorrect)
    pygame.display.flip()
