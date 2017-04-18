//
//  GameViewController.m
//  laidoff
//
//  Created by 김거엽 on 2017. 4. 15..
//  Copyright © 2017년 KIMGEO YEOB. All rights reserved.
//

#import "GameViewController.h"
#import "lwgl.h"
#import "laidoff.h"

@interface GameViewController () {
  
}
@property (strong, nonatomic) EAGLContext *context;
@property (nonatomic) LWCONTEXT *pLwc;

- (void)setupGL;
- (void)tearDownGL;

@end

@implementation GameViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat16;
    
    view.userInteractionEnabled = YES;
    self.preferredFramesPerSecond = 60;
    
    [self setupGL];
    
    self.pLwc = lw_init();
    lw_set_size(self.pLwc, 1920, 1080);
}

- (void)dealloc
{    
    [self tearDownGL];
    
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];

    if ([self isViewLoaded] && ([[self view] window] == nil)) {
        self.view = nil;
        
        [self tearDownGL];
        
        if ([EAGLContext currentContext] == self.context) {
            [EAGLContext setCurrentContext:nil];
        }
        self.context = nil;
    }

    // Dispose of any resources that can be recreated.
}

- (BOOL)prefersStatusBarHidden {
    return YES;
}

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    lwc_update(self.pLwc, self.timeSinceLastUpdate);
}

static CGPoint getNormalizedPoint(UIView* view, CGPoint locationInView)
{
    const float normalizedX = (locationInView.x / view.bounds.size.width) * 2.f - 1.f;
    const float normalizedY = -((locationInView.y / view.bounds.size.height) * 2.f - 1.f);
    return CGPointMake(normalizedX, normalizedY);
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesBegan:touches withEvent:event];
    UITouch* touchEvent = [touches anyObject];
    CGPoint locationInView = [touchEvent locationInView:self.view];
    CGPoint normalizedPoint = getNormalizedPoint(self.view, locationInView);
    //on_touch_press(normalizedPoint.x, normalizedPoint.y);
    lw_trigger_mouse_press(self.pLwc, normalizedPoint.x, normalizedPoint.y);
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesMoved:touches withEvent:event];
    UITouch* touchEvent = [touches anyObject];
    CGPoint locationInView = [touchEvent locationInView:self.view];
    CGPoint normalizedPoint = getNormalizedPoint(self.view, locationInView);
    //on_touch_drag(normalizedPoint.x, normalizedPoint.y);
    lw_trigger_mouse_move(self.pLwc, normalizedPoint.x, normalizedPoint.y);
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesEnded:touches withEvent:event];
    UITouch* touchEvent = [touches anyObject];
    CGPoint locationInView = [touchEvent locationInView:self.view];
    CGPoint normalizedPoint = getNormalizedPoint(self.view, locationInView);
    lw_trigger_mouse_release(self.pLwc, normalizedPoint.x, normalizedPoint.y);
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesCancelled:touches withEvent:event];
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    lwc_render(self.pLwc);
}

@end
